// Phase 14 (part 3) — the sampler reaches the model.
//
// The load-bearing tests here are the wiring, not the sampler's sound (that
// is SamplerTests.cpp): a channel that *declares* itself a sampler with zones
// must come out of the project compiler as a playing sampler, its assets
// resolved through the shared cache, and every failure along that path must
// degrade to a warning and silence rather than a failed compile.

#include "doctest.h"

#include "engine/audio/SampleCache.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/ProjectGraphCompiler.h"

#include <filesystem>
#include <string>
#include <vector>

using namespace incdaw;
using incdaw::engine::FrameCount;
using incdaw::engine::Tick;

namespace fs = std::filesystem;

namespace {

/// A temporary directory that removes itself, so a failing test cannot leave
/// state behind that makes the next run pass or fail for the wrong reason.
struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path()
               / ("incdaw-sampler-wiring-" + name + "-" + std::to_string(nextSerial())))
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

/// A short, unmistakably non-silent mono file.
fs::path writeTestWav(const fs::path& directory, const char* name, double sampleRate,
                      FrameCount frames = 4800)
{
    engine::AudioFileData data;
    data.sampleRate   = sampleRate;
    data.channelCount = 1;
    data.frameCount   = frames;
    data.channels.assign(1, std::vector<engine::Sample>(static_cast<std::size_t>(frames), 0.5f));

    const fs::path path = directory / name;
    REQUIRE(engine::WavFile::write(path, data));
    return path;
}

project::MidiEvent noteAtZero(int key = 60)
{
    project::MidiEvent event;
    event.type     = project::MidiEventType::note;
    event.tick     = 0;
    event.duration = engine::ticksPerQuarterNote;
    event.key      = key;
    event.value    = 100;
    return event;
}

/// One channel, one pattern with a note at tick 0, and an asset for `wav`.
struct SamplerProject {
    project::Project project;
    project::EntityId channel;
    project::EntityId asset;

    explicit SamplerProject(const fs::path& wav)
    {
        auto& added = project.addChannel("Kick");
        added.instrument = plugins::builtinSampler();
        channel          = added.id;

        auto& audioAsset        = project.addAudioAsset(wav.string());
        audioAsset.absolutePath = wav.string();
        asset                   = audioAsset.id;

        project::ChannelSamplerZone zone;
        zone.asset   = asset;
        zone.rootKey = 60;
        project.findChannel(channel)->samplerZones.push_back(zone);

        auto& pattern = project.addPattern("Pattern");
        pattern.contentFor(channel).events.push_back(noteAtZero());
    }
};

engine::Sample renderedPeak(const project::CompiledProjectGraph& compiled)
{
    engine::AudioBufferPool output;
    output.allocate(1, 2, 256);
    compiled.graph->process(output.buffer(0), 256, 0);
    return output.buffer(0).peak();
}

} // namespace

TEST_CASE("the sample cache hands out one decode per unchanged file")
{
    ScratchDirectory scratch{"cache"};
    const fs::path wav = writeTestWav(scratch.path, "tone.wav", 48000.0);

    engine::SampleCache cache;
    std::string         error;

    const auto first = cache.load(wav, error);
    REQUIRE(first != nullptr);
    CHECK(first->frameCount == 4800);

    const auto second = cache.load(wav, error);
    CHECK(second.get() == first.get());
    CHECK(cache.entryCount() == 1);

    SUBCASE("a changed file is decoded afresh")
    {
        // A different length changes the size, which is half the identity key;
        // relying on mtime alone would race the filesystem's timestamp
        // granularity on a fast rewrite.
        writeTestWav(scratch.path, "tone.wav", 48000.0, 2400);

        const auto third = cache.load(wav, error);
        REQUIRE(third != nullptr);
        CHECK(third.get() != first.get());
        CHECK(third->frameCount == 2400);
        CHECK(cache.entryCount() == 1);
    }

    SUBCASE("clear() empties the cache")
    {
        cache.clear();
        CHECK(cache.entryCount() == 0);
    }
}

TEST_CASE("a failed load is reported and not cached")
{
    ScratchDirectory scratch{"cache-miss"};

    engine::SampleCache cache;
    std::string         error;

    const fs::path missing = scratch.path / "missing.wav";
    CHECK(cache.load(missing, error) == nullptr);
    CHECK(!error.empty());
    CHECK(cache.entryCount() == 0);

    // The file appearing later — a relink, a restored disk — must succeed on
    // the next try rather than being remembered as broken.
    writeTestWav(scratch.path, "missing.wav", 48000.0);

    error.clear();
    CHECK(cache.load(missing, error) != nullptr);
}

TEST_CASE("a sampler channel compiles into a playing sampler")
{
    ScratchDirectory scratch{"wiring"};
    SamplerProject   fixture{writeTestWav(scratch.path, "kick.wav", 48000.0)};

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 256;

    const auto compiled = project::compileProjectGraph(fixture.project, map, options);

    REQUIRE(compiled);
    CHECK(compiled.warnings.empty());
    REQUIRE(compiled.instrumentFor(fixture.channel) != nullptr);
    CHECK(renderedPeak(compiled) > 0.0f);
}

TEST_CASE("zone assets resolve through the shared cache across rebuilds")
{
    ScratchDirectory scratch{"wiring-cache"};
    SamplerProject   fixture{writeTestWav(scratch.path, "kick.wav", 48000.0)};

    const engine::TempoMap map{120.0, 48000.0};
    engine::SampleCache    cache;

    project::GraphCompileOptions options;
    options.maxBlockSize = 256;
    options.sampleCache  = &cache;

    const auto first = project::compileProjectGraph(fixture.project, map, options);
    REQUIRE(first);
    CHECK(cache.entryCount() == 1);

    // The rebuild that follows every edit: same file, no second decode.
    const auto second = project::compileProjectGraph(fixture.project, map, options);
    REQUIRE(second);
    CHECK(cache.entryCount() == 1);
    CHECK(renderedPeak(second) > 0.0f);
}

TEST_CASE("a sampler zone at a different sample rate still plays")
{
    // Clips refuse cross-rate assets; the sampler must not, because it
    // repitches by rate anyway. A 24 kHz drum in a 48 kHz session is a
    // completely ordinary sampler situation.
    ScratchDirectory scratch{"wiring-rate"};
    SamplerProject   fixture{writeTestWav(scratch.path, "kick24.wav", 24000.0)};

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 256;

    const auto compiled = project::compileProjectGraph(fixture.project, map, options);

    REQUIRE(compiled);
    CHECK(compiled.warnings.empty());
    CHECK(renderedPeak(compiled) > 0.0f);
}

TEST_CASE("a sampler whose asset is missing is silent with a warning, not an error")
{
    ScratchDirectory scratch{"wiring-missing"};
    SamplerProject   fixture{scratch.path / "never-written.wav"};

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 256;

    const auto compiled = project::compileProjectGraph(fixture.project, map, options);

    REQUIRE(compiled);
    CHECK(!compiled.warnings.empty());

    // The channel is present — a missing sample must not evict the channel
    // from the graph, or reconnecting the file would need a topology change.
    REQUIRE(compiled.instrumentFor(fixture.channel) != nullptr);
    CHECK(renderedPeak(compiled) == doctest::Approx(0.0f));
}

TEST_CASE("a zone naming an asset the project does not have warns too")
{
    ScratchDirectory scratch{"wiring-unknown-asset"};
    SamplerProject   fixture{writeTestWav(scratch.path, "kick.wav", 48000.0)};

    fixture.project.findChannel(fixture.channel)->samplerZones[0].asset =
        project::EntityId{9999};

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 256;

    const auto compiled = project::compileProjectGraph(fixture.project, map, options);

    REQUIRE(compiled);
    CHECK(!compiled.warnings.empty());
    CHECK(renderedPeak(compiled) == doctest::Approx(0.0f));
}

TEST_CASE("an unknown builtin uid is a warning and a silent channel")
{
    ScratchDirectory scratch{"wiring-unknown-builtin"};
    SamplerProject   fixture{writeTestWav(scratch.path, "kick.wav", 48000.0)};

    fixture.project.findChannel(fixture.channel)->instrument = {plugins::Format::builtin,
                                                               "incdaw.does-not-exist"};

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 256;

    const auto compiled = project::compileProjectGraph(fixture.project, map, options);

    REQUIRE(compiled);
    CHECK(!compiled.warnings.empty());
    CHECK(compiled.instrumentFor(fixture.channel) == nullptr);
}

TEST_CASE("the builtin synth identity builds a SimpleSynth")
{
    project::Project project;

    auto& channel      = project.addChannel("Lead");
    channel.instrument = plugins::builtinSimpleSynth();
    const auto id      = channel.id;

    auto& pattern = project.addPattern("Pattern");
    pattern.contentFor(id).events.push_back(noteAtZero());

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 256;

    const auto compiled = project::compileProjectGraph(project, map, options);

    REQUIRE(compiled);
    CHECK(compiled.warnings.empty());
    REQUIRE(compiled.instrumentFor(id) != nullptr);
    CHECK(renderedPeak(compiled) > 0.0f);
}
