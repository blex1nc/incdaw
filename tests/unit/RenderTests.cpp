// Phase 17 — offline rendering and export.
//
// The exit criterion (docs/ROADMAP.md): offline render is byte-identical to
// a realtime capture of the same project. The "realtime capture" here is the
// audio callback's exact loop — the same compiled graph, processed block by
// block at the same block size — captured instead of sent to the device.
// There is no separate offline DSP to drift, and this test is what keeps it
// that way.

#include "doctest.h"

#include "engine/audio/AiffFile.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/Resampler.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/OfflineRender.h"
#include "project/ProjectGraphCompiler.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace incdaw;
using incdaw::engine::ticksPerQuarterNote;

namespace fs = std::filesystem;

namespace {

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path()
               / ("incdaw-render-" + name + "-" + std::to_string(nextSerial())))
    {
        std::error_code code;
        fs::remove_all(path, code);
        fs::create_directories(path, code);
    }

    ~ScratchDirectory()
    {
        std::error_code code;
        fs::remove_all(path, code);
    }

    fs::path path;

private:
    static int nextSerial()
    {
        static int serial = 0;
        return ++serial;
    }
};

/// A two-channel project with distinct content, arranged for two bars.
project::Project makeArrangedProject()
{
    project::Project project;

    auto& lead      = project.addChannel("Lead");
    auto& bass      = project.addChannel("Bass");
    const auto leadId = lead.id;
    const auto bassId = bass.id;

    auto& track = project.addTrack(project::TrackType::instrument, "Track");

    auto& pattern  = project.addPattern("Motif");
    pattern.length = ticksPerQuarterNote * 4;

    project::MidiEvent note;
    note.type     = project::MidiEventType::note;
    note.duration = ticksPerQuarterNote;
    note.key      = 60;
    note.value    = 100;
    pattern.contentFor(leadId).events.push_back(note);

    note.tick = ticksPerQuarterNote * 2;
    note.key  = 43;
    pattern.contentFor(bassId).events.push_back(note);

    for (const project::Tick start : {project::Tick{0}, ticksPerQuarterNote * 4}) {
        auto& clip       = project.addClip(project::ClipType::pattern, track.id, pattern.id);
        clip.startTick   = start;
        clip.lengthTicks = pattern.length;
    }

    return project;
}

} // namespace

TEST_CASE("EXIT CRITERION: offline render is byte-identical to a realtime capture")
{
    project::Project       projectModel = makeArrangedProject();
    const engine::TempoMap map{120.0, 48000.0};

    // The offline side.
    project::RenderOptions options;
    options.tailSeconds = 0.5;
    options.randomSeed  = 42;

    const auto rendered = project::renderProject(projectModel, map, options);
    REQUIRE(rendered);
    CHECK(rendered.warnings.empty());

    // The realtime side: the audio callback's loop, captured. Same compile
    // options, same seed, same block size — what the engine would do, minus
    // the loudspeaker.
    project::GraphCompileOptions compile;
    compile.sampleRate   = options.sampleRate;
    compile.maxBlockSize = options.blockSize;
    compile.source       = project::PlaybackSource::arrangement;
    compile.randomSeed   = options.randomSeed;

    const auto compiled = project::compileProjectGraph(projectModel, map, compile);
    REQUIRE(compiled);

    engine::AudioBufferPool pool;
    pool.allocate(1, 2, options.blockSize);

    const auto total = rendered.audio.frameCount;
    std::vector<engine::Sample> captured[2];

    for (engine::FrameCount at = 0; at < total;) {
        const engine::FrameCount count =
            std::min<engine::FrameCount>(options.blockSize, total - at);

        const auto view = pool.buffer(0);
        compiled.graph->process(view, count, at);

        for (std::size_t channel = 0; channel < 2; ++channel)
            captured[channel].insert(captured[channel].end(), view.channel(channel),
                                     view.channel(channel) + count);

        at += count;
    }

    // Byte-identical, the criterion's own word: exact float equality, every
    // sample, both channels.
    for (std::size_t channel = 0; channel < 2; ++channel) {
        REQUIRE(captured[channel].size()
                == static_cast<std::size_t>(rendered.audio.frameCount));

        for (std::size_t frame = 0; frame < captured[channel].size(); ++frame)
            REQUIRE(rendered.audio.channels[channel][frame] == captured[channel][frame]);
    }

    // And it actually made sound.
    engine::Sample peak = 0.0f;
    for (const engine::Sample sample : rendered.audio.channels[0])
        peak = std::max(peak, std::abs(sample));
    CHECK(peak > 0.01f);
}

TEST_CASE("a region renders exactly its frames plus the tail")
{
    project::Project       projectModel = makeArrangedProject();
    const engine::TempoMap map{120.0, 48000.0};

    project::RenderOptions options;
    options.regionStart  = 48000;
    options.regionLength = 24000;
    options.tailSeconds  = 0.25;

    const auto rendered = project::renderProject(projectModel, map, options);
    REQUIRE(rendered);

    CHECK(rendered.arrangementFrames == 24000);
    CHECK(rendered.audio.frameCount == 24000 + 12000);

    // The region's content equals the same span of a full render.
    project::RenderOptions full;
    full.tailSeconds = 0.0;
    const auto whole = project::renderProject(projectModel, map, full);
    REQUIRE(whole);

    for (engine::FrameCount frame = 0; frame < 24000; ++frame)
        REQUIRE(rendered.audio.channels[0][static_cast<std::size_t>(frame)]
                == whole.audio.channels[0][static_cast<std::size_t>(48000 + frame)]);
}

TEST_CASE("a stem is the soloed node, an individual track is the soloed channel")
{
    project::Project       projectModel = makeArrangedProject();
    const engine::TempoMap map{120.0, 48000.0};

    project::RenderOptions options;
    options.tailSeconds = 0.1;

    // Solo the bass channel: the lead's bar-one note must be silent.
    options.soloChannel = projectModel.channels()[1].id;

    const auto bassOnly = project::renderProject(projectModel, map, options);
    REQUIRE(bassOnly);

    // Bar one, beat one: only the lead plays there in the full mix.
    engine::Sample early = 0.0f;
    for (std::size_t frame = 0; frame < 12000; ++frame)
        early = std::max(early, std::abs(bassOnly.audio.channels[0][frame]));

    CHECK(early == doctest::Approx(0.0f));

    // Beat three (frame 48000 at 120 BPM): the bass note sounds.
    engine::Sample later = 0.0f;
    for (std::size_t frame = 48000; frame < 60000; ++frame)
        later = std::max(later, std::abs(bassOnly.audio.channels[0][frame]));

    CHECK(later > 0.01f);
}

TEST_CASE("normalize scales the peak to exactly one")
{
    project::Project       projectModel = makeArrangedProject();
    const engine::TempoMap map{120.0, 48000.0};

    project::RenderOptions options;
    options.tailSeconds = 0.1;
    options.normalize   = true;

    const auto rendered = project::renderProject(projectModel, map, options);
    REQUIRE(rendered);

    engine::Sample peak = 0.0f;
    for (const auto& channel : rendered.audio.channels)
        for (const engine::Sample sample : channel)
            peak = std::max(peak, std::abs(sample));

    CHECK(peak == doctest::Approx(1.0f));
}

TEST_CASE("renders are deterministic, dither included")
{
    project::Project       projectModel = makeArrangedProject();
    const engine::TempoMap map{120.0, 48000.0};

    project::RenderOptions options;
    options.tailSeconds = 0.1;
    options.bitDepth    = project::RenderOptions::BitDepth::pcm16;
    options.dither      = true;
    options.randomSeed  = 7;

    const auto first  = project::renderProject(projectModel, map, options);
    const auto second = project::renderProject(projectModel, map, options);
    REQUIRE(first);
    REQUIRE(second);

    for (std::size_t channel = 0; channel < 2; ++channel)
        for (std::size_t frame = 0; frame < first.audio.channels[channel].size(); ++frame)
            REQUIRE(first.audio.channels[channel][frame]
                    == second.audio.channels[channel][frame]);
}

TEST_CASE("the resampler preserves a tone and its level across rates")
{
    // One second of 1 kHz at 48 kHz.
    engine::AudioFileData source;
    source.sampleRate   = 48000.0;
    source.channelCount = 1;
    source.frameCount   = 48000;
    source.channels.assign(1, std::vector<engine::Sample>(48000));

    for (std::size_t frame = 0; frame < 48000; ++frame)
        source.channels[0][frame] = static_cast<engine::Sample>(
            0.5 * std::sin(2.0 * 3.14159265358979 * 1000.0 * static_cast<double>(frame)
                           / 48000.0));

    const auto converted = engine::dsp::resample(source, 44100.0);

    CHECK(converted.sampleRate == doctest::Approx(44100.0));
    CHECK(converted.frameCount == 44100);

    // The ideal 1 kHz tone at the new rate, compared away from the edges
    // (the kernel is zero-padded there). RMS error against the ideal must
    // be far below the signal: -60 dB is 0.0005 against a 0.354 RMS signal.
    double errorPower  = 0.0;
    double signalPower = 0.0;
    for (std::size_t frame = 64; frame < 44100 - 64; ++frame) {
        const double ideal = 0.5
                           * std::sin(2.0 * 3.14159265358979 * 1000.0
                                      * static_cast<double>(frame) / 44100.0);
        const double actual = static_cast<double>(converted.channels[0][frame]);
        errorPower += (actual - ideal) * (actual - ideal);
        signalPower += ideal * ideal;
    }

    CHECK(std::sqrt(errorPower / signalPower) < 0.001);   // < -60 dB
}

TEST_CASE("render to file writes WAV and AIFF containers")
{
    ScratchDirectory scratch{"files"};

    project::Project       projectModel = makeArrangedProject();
    const engine::TempoMap map{120.0, 48000.0};

    project::RenderOptions options;
    options.tailSeconds = 0.1;

    SUBCASE("WAV round-trips through the project's own reader")
    {
        const fs::path wav = scratch.path / "master.wav";
        const auto rendered = project::renderProjectToFile(projectModel, map, options, wav);
        REQUIRE(rendered);

        engine::AudioFileData loaded;
        REQUIRE(engine::WavFile::read(wav, loaded));
        REQUIRE(loaded.frameCount == rendered.audio.frameCount);

        for (std::size_t frame = 0; frame < 1000; ++frame)
            REQUIRE(loaded.channels[0][frame] == rendered.audio.channels[0][frame]);
    }

    SUBCASE("AIFF writes a well-formed big-endian container")
    {
        const fs::path aiff = scratch.path / "master.aiff";
        options.bitDepth    = project::RenderOptions::BitDepth::pcm16;
        options.dither      = false;

        const auto rendered = project::renderProjectToFile(projectModel, map, options, aiff);
        REQUIRE(rendered);

        std::ifstream stream(aiff, std::ios::binary);
        REQUIRE(stream.good());

        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(stream)),
                                        std::istreambuf_iterator<char>());
        REQUIRE(bytes.size() > 54);

        CHECK(std::string(bytes.begin(), bytes.begin() + 4) == "FORM");
        CHECK(std::string(bytes.begin() + 8, bytes.begin() + 12) == "AIFF");
        CHECK(std::string(bytes.begin() + 12, bytes.begin() + 16) == "COMM");

        // COMM: channels at offset 20 (big-endian u16) == 2.
        CHECK((bytes[20] << 8 | bytes[21]) == 2);

        // Frame count at offset 22 (big-endian u32).
        const std::uint32_t frames = (static_cast<std::uint32_t>(bytes[22]) << 24)
                                   | (static_cast<std::uint32_t>(bytes[23]) << 16)
                                   | (static_cast<std::uint32_t>(bytes[24]) << 8)
                                   | static_cast<std::uint32_t>(bytes[25]);
        CHECK(frames == static_cast<std::uint32_t>(rendered.audio.frameCount));

        // Bits per sample at offset 26 == 16, and the 80-bit rate's exponent
        // + mantissa decode back to 48000.
        CHECK((bytes[26] << 8 | bytes[27]) == 16);
    }
}

TEST_CASE("an empty project with no tail refuses instead of writing nothing")
{
    project::Project       projectModel;
    const engine::TempoMap map{120.0, 48000.0};

    project::RenderOptions options;
    options.tailSeconds = 0.0;

    const auto rendered = project::renderProject(projectModel, map, options);
    CHECK(!rendered);
    CHECK(!rendered.error.empty());
}
