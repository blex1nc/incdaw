// Phase 14 (part 6) — streamed sampler zones, and the phase's exit criterion.
//
// docs/ROADMAP.md: "a multisampled instrument plays correctly across the
// keyboard with velocity layers, streaming from disk without underruns."
// That sentence is the load-bearing test here, measured rather than claimed:
// the streams count their own underruns, the layers are files with distinct
// constants so the output names the layer that played, and "across the
// keyboard" is a chord of held notes above and below the root, each reading
// the disk at its own rate.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/instrument/Sampler.h"
#include "engine/midi/MidiBuffer.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/ProjectGraphCompiler.h"

#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace fs = std::filesystem;

namespace {

constexpr FrameCount blockSize = 256;

/// Small enough that tests cross the RAM-to-stream seam in a few dozen
/// blocks, large enough to cover the steering latency the pool depends on.
constexpr FrameCount testHeadFrames    = 4096;
constexpr FrameCount testSegmentFrames = 8192;

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path()
               / ("incdaw-sampler-streaming-" + name + "-" + std::to_string(nextSerial())))
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

/// A long mono file holding one constant: whoever hears `value` knows which
/// file — which layer — played, at any point in the file.
fs::path writeConstantWav(const fs::path& directory, const char* name, Sample value,
                          FrameCount frames)
{
    AudioFileData data;
    data.sampleRate   = 48000.0;
    data.channelCount = 1;
    data.frameCount   = frames;
    data.channels.assign(1, std::vector<Sample>(static_cast<std::size_t>(frames), value));

    const fs::path path = directory / name;
    REQUIRE(WavFile::write(path, data));
    return path;
}

std::shared_ptr<SamplerZoneStream> openZoneStream(const fs::path& path)
{
    std::string error;
    auto stream = SamplerZoneStream::create(path, testHeadFrames, error, testSegmentFrames);
    REQUIRE_MESSAGE(stream != nullptr, error);
    return stream;
}

SamplerZone streamedZone(std::shared_ptr<SamplerZoneStream> stream, int velocityLow,
                         int velocityHigh)
{
    SamplerZone zone;
    zone.sample       = stream->head();
    zone.stream       = std::move(stream);
    zone.velocityLow  = velocityLow;
    zone.velocityHigh = velocityHigh;
    return zone;
}

/// Renders `blocks` blocks, servicing the streamer between each the way the
/// real service thread does — but deterministically.
struct StreamedRender {
    std::vector<Sample> left;
    bool                finite = true;
};

StreamedRender renderStreamed(Sampler& sampler, const MidiBuffer& firstBlock, int blocks,
                              DiskStreamer& streamer)
{
    StreamedRender result;
    result.left.reserve(static_cast<std::size_t>(blocks) * static_cast<std::size_t>(blockSize));

    AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    MidiBuffer empty;

    for (int block = 0; block < blocks; ++block) {
        const auto output = pool.buffer(0);
        output.clear();

        sampler.processBlock(output, block == 0 ? firstBlock : empty);
        streamer.serviceOnce();

        for (FrameCount frame = 0; frame < blockSize; ++frame) {
            const Sample value = output.channel(0)[frame];
            result.finite      = result.finite && std::isfinite(value);
            result.left.push_back(value);
        }
    }

    return result;
}

double rmsOver(const std::vector<Sample>& samples, std::size_t from, std::size_t count)
{
    double sum = 0.0;
    for (std::size_t index = from; index < from + count && index < samples.size(); ++index)
        sum += static_cast<double>(samples[index]) * static_cast<double>(samples[index]);

    return std::sqrt(sum / static_cast<double>(count));
}

} // namespace

TEST_CASE("EXIT CRITERION: velocity layers across the keyboard stream without underruns")
{
    ScratchDirectory scratch{"exit"};

    // Two layers, 6.5 s each: a +12 note reads the source at double rate, so
    // the file must outlast the render at the highest key under test.
    const FrameCount fileFrames = 312000;
    const fs::path soft = writeConstantWav(scratch.path, "soft.wav", 0.2f, fileFrames);
    const fs::path loud = writeConstantWav(scratch.path, "loud.wav", 0.4f, fileFrames);

    auto softStream = openZoneStream(soft);
    auto loudStream = openZoneStream(loud);

    DiskStreamer streamer;
    softStream->registerWith(streamer);
    loudStream->registerWith(streamer);

    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    sampler.setAttackSeconds(0.0);
    sampler.setDecaySeconds(0.0);
    sampler.setSustainLevel(1.0);
    sampler.setReleaseSeconds(0.0);
    sampler.setZones({streamedZone(softStream, 1, 64), streamedZone(loudStream, 65, 127)});

    // The chord: below the root, at it, and an octave above — three read
    // rates against the same pool — split across both velocity layers.
    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 48, 40, 0));    // soft layer, half rate
    midi.insert(MidiMessage::noteOn(0, 60, 100, 0));   // loud layer, unity
    midi.insert(MidiMessage::noteOn(0, 72, 40, 0));    // soft layer, double rate

    // 3 s: the head is 4096 frames ≈ 85 ms, so ~97% of this is streamed.
    const int blocks = 563;
    const auto rendered = renderStreamed(sampler, midi, blocks, streamer);

    CHECK(rendered.finite);

    // The exit criterion's own words: no underruns.
    CHECK(softStream->underrunCount() == 0);
    CHECK(loudStream->underrunCount() == 0);

    // Playing "correctly": with transparent envelopes and constant sources,
    // the mix must sit at the sum of the right layers' constants, scaled by
    // velocity — soft 0.2·(40/127) twice, loud 0.4·(100/127) once — deep into
    // streamed territory, at the very end of the render.
    const double expected = 2.0 * 0.2 * (40.0 / 127.0) + 0.4 * (100.0 / 127.0);

    const std::size_t total = rendered.left.size();
    const double lateRms = rmsOver(rendered.left, total - blockSize * 8, blockSize * 8);

    CHECK(lateRms == doctest::Approx(expected).epsilon(0.02));

    CHECK(sampler.activeVoiceCount() == 3);
}

TEST_CASE("a streamed note starts instantly from the head")
{
    ScratchDirectory scratch{"instant"};
    const fs::path wav = writeConstantWav(scratch.path, "pad.wav", 0.5f, 96000);

    auto zoneStream = openZoneStream(wav);

    DiskStreamer streamer;
    zoneStream->registerWith(streamer);

    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    sampler.setAttackSeconds(0.0);
    sampler.setDecaySeconds(0.0);
    sampler.setSustainLevel(1.0);
    sampler.setZones({streamedZone(zoneStream, 1, 127)});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto rendered = renderStreamed(sampler, midi, 1, streamer);

    // The very first block sounds — no waiting for a disk window.
    CHECK(rmsOver(rendered.left, 0, blockSize) == doctest::Approx(0.5).epsilon(0.01));
}

TEST_CASE("more held notes than pool slots degrade to head-only, never block")
{
    ScratchDirectory scratch{"exhaust"};
    const fs::path wav = writeConstantWav(scratch.path, "pad.wav", 0.25f, 96000);

    auto zoneStream = openZoneStream(wav);

    DiskStreamer streamer;
    zoneStream->registerWith(streamer);

    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    sampler.setAttackSeconds(0.0);
    sampler.setDecaySeconds(0.0);
    sampler.setSustainLevel(1.0);
    sampler.setZones({streamedZone(zoneStream, 1, 127)});

    // Six held notes against four pool slots.
    MidiBuffer midi;
    for (int key : {55, 57, 59, 60, 62, 64})
        midi.insert(MidiMessage::noteOn(0, key, 100, 0));

    // Far enough that every voice is well past the head.
    const auto rendered = renderStreamed(sampler, midi, 120, streamer);

    CHECK(rendered.finite);
    CHECK(sampler.activeVoiceCount() == 6);

    // The four slot-holding voices are still audible past the head; the two
    // head-only voices have gone silent rather than blocking or allocating.
    const std::size_t total = rendered.left.size();
    CHECK(rmsOver(rendered.left, total - blockSize * 4, blockSize * 4) > 0.1);

    // Releasing every note returns every slot: a fresh chord claims four
    // slots again, so slot release on voice end demonstrably works.
    sampler.allNotesOff();

    MidiBuffer again;
    for (int key : {60, 64, 67, 71})
        again.insert(MidiMessage::noteOn(0, key, 100, 0));

    const auto second = renderStreamed(sampler, again, 120, streamer);
    const std::size_t secondTotal = second.left.size();
    CHECK(rmsOver(second.left, secondTotal - blockSize * 4, blockSize * 4) > 0.1);
}

TEST_CASE("the compiler streams a long sampler zone end to end")
{
    ScratchDirectory scratch{"compile"};
    const fs::path wav = writeConstantWav(scratch.path, "long.wav", 0.3f, 192000);

    project::Project project;

    auto& channel      = project.addChannel("Pad");
    channel.instrument = plugins::builtinSampler();
    const auto channelId = channel.id;

    auto& asset        = project.addAudioAsset(wav.string());
    asset.absolutePath = wav.string();

    project::ChannelSamplerZone spec;
    spec.asset = asset.id;
    project.findChannel(channelId)->samplerZones.push_back(spec);

    auto& pattern = project.addPattern("P");
    project::MidiEvent note;
    note.type     = project::MidiEventType::note;
    note.tick     = 0;
    note.duration = engine::ticksPerQuarterNote * 16;   // held for 8 s at 120
    note.key      = 60;
    note.value    = 100;
    pattern.contentFor(channelId).events.push_back(note);

    const engine::TempoMap map{120.0, 48000.0};

    DiskStreamer streamer;
    SampleCache  cache;

    project::GraphCompileOptions options;
    options.maxBlockSize             = blockSize;
    options.diskStreamer             = &streamer;
    options.sampleCache              = &cache;
    options.streamingThresholdFrames = 48000;      // 1 s: the 4 s file streams
    options.samplerHeadFrames        = testHeadFrames;

    const auto compiled = project::compileProjectGraph(project, map, options);

    REQUIRE(compiled);
    CHECK(compiled.warnings.empty());

    // A streamed zone must not have gone through the preload cache.
    CHECK(cache.entryCount() == 0);

    engine::AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    // Two seconds — far past the 85 ms head — servicing as we go.
    double latePeak = 0.0;
    for (int block = 0; block < 375; ++block) {
        const auto output = pool.buffer(0);
        compiled.graph->process(output, blockSize,
                                static_cast<FramePosition>(block) * blockSize);
        streamer.serviceOnce();

        if (block > 360)
            latePeak = std::max(latePeak, static_cast<double>(output.peak()));
    }

    // The exact expected level, not a vague threshold: source 0.3, velocity
    // 100/127, the default master headroom 0.8, and two constant-power pan
    // stages at centre (channel strip and master strip, 1/√2 each → 0.5).
    CHECK(latePeak == doctest::Approx(0.3 * (100.0 / 127.0) * 0.8 * 0.5).epsilon(0.05));
}
