// Phase 12 (part 4) — the disk streamer.
//
// The load-bearing property is equivalence: a streamed clip must produce the
// same samples as the same clip preloaded, bit-exactly, with the window
// refilling under it. The failure mode must be honest: a starved window
// serves counted silence, and never blocks the caller.

#include "doctest.h"

#include "engine/audio/AudioStream.h"
#include "engine/audio/WavFile.h"
#include "engine/audio/WavStreamReader.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "project/Model.h"
#include "project/ProjectGraphCompiler.h"

#include <cmath>
#include <filesystem>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
namespace fs = std::filesystem;

namespace {

struct ScratchDir {
    fs::path path;

    explicit ScratchDir(const char* name)
        : path(fs::temp_directory_path() / name)
    {
        std::error_code code;
        fs::remove_all(path, code);
        fs::create_directories(path, code);
    }

    ~ScratchDir()
    {
        std::error_code code;
        fs::remove_all(path, code);
    }
};

Sample tone(FrameCount frame, std::size_t channel = 0)
{
    return static_cast<Sample>(0.2 + 0.5 * std::sin(0.031 * static_cast<double>(frame)
                                                    + static_cast<double>(channel)));
}

AudioFileData makeAudio(std::size_t channels, FrameCount frames, double sampleRate = 48000.0)
{
    AudioFileData data;
    data.sampleRate   = sampleRate;
    data.channelCount = channels;
    data.frameCount   = frames;
    data.channels.assign(channels, std::vector<Sample>(static_cast<std::size_t>(frames)));

    for (std::size_t channel = 0; channel < channels; ++channel)
        for (FrameCount frame = 0; frame < frames; ++frame)
            data.channels[channel][static_cast<std::size_t>(frame)] = tone(frame, channel);

    return data;
}

} // namespace

// ── WavStreamReader ──────────────────────────────────────────────────────────

TEST_CASE("random-access reads agree with the whole-file reader, all formats")
{
    ScratchDir scratch{"incdaw-streamreader"};

    const auto data = makeAudio(2, 10000);

    for (const auto format : {WavFile::Format::float32, WavFile::Format::pcm16,
                              WavFile::Format::pcm24}) {
        const fs::path path = scratch.path / "probe.wav";
        REQUIRE(bool(WavFile::write(path, data, format)));

        AudioFileData whole;
        REQUIRE(bool(WavFile::read(path, whole)));

        WavStreamReader reader;
        REQUIRE(bool(reader.open(path)));
        REQUIRE(reader.frameCount() == 10000);
        REQUIRE(reader.channelCount() == 2);
        REQUIRE(reader.sampleRate() == 48000.0);

        // Slices from the start, the middle, straddling the end, and fully
        // past the end. Same decoder both sides, so equality is bit-exact.
        const FrameCount offsets[] = {0, 1, 4999, 9900, 12000};

        std::vector<Sample> left(300), right(300);
        Sample* channels[] = {left.data(), right.data()};

        for (const FrameCount offset : offsets) {
            REQUIRE(reader.readAt(offset, 300, channels, 2));

            for (FrameCount frame = 0; frame < 300; ++frame) {
                const FrameCount source = offset + frame;
                const Sample expectedL = source < 10000
                    ? whole.channels[0][static_cast<std::size_t>(source)] : 0.0f;
                const Sample expectedR = source < 10000
                    ? whole.channels[1][static_cast<std::size_t>(source)] : 0.0f;

                REQUIRE(left[static_cast<std::size_t>(frame)] == expectedL);
                REQUIRE(right[static_cast<std::size_t>(frame)] == expectedR);
            }
        }
    }
}

// ── AudioStream ──────────────────────────────────────────────────────────────

TEST_CASE("a serviced stream reproduces the file bit-exactly across refills")
{
    ScratchDir scratch{"incdaw-stream-serviced"};

    constexpr FrameCount total = 5000;
    const auto data = makeAudio(1, total);
    REQUIRE(bool(WavFile::write(scratch.path / "long.wav", data)));

    AudioStream stream;
    REQUIRE(bool(stream.open(scratch.path / "long.wav", 256)));   // tiny segments: many refills
    stream.prefill(0);

    std::vector<Sample> out(128);

    for (FrameCount position = 0; position < total; position += 128) {
        stream.read(position, 128, 0, out.data());

        for (FrameCount frame = 0; frame < 128 && position + frame < total; ++frame)
            REQUIRE(out[static_cast<std::size_t>(frame)] == tone(position + frame));

        // The service thread in real life; called synchronously here so the
        // test is deterministic rather than timing-dependent.
        stream.service();
    }

    CHECK(stream.underrunCount() == 0);
}

TEST_CASE("a seek is served after one service pass, and counted before it")
{
    ScratchDir scratch{"incdaw-stream-seek"};

    const auto data = makeAudio(1, 8000);
    REQUIRE(bool(WavFile::write(scratch.path / "seek.wav", data)));

    AudioStream stream;
    REQUIRE(bool(stream.open(scratch.path / "seek.wav", 256)));
    stream.prefill(0);

    std::vector<Sample> out(128);

    // Far outside the window: honest silence, counted.
    stream.read(6000, 128, 0, out.data());
    CHECK(stream.underrunCount() == 1);
    for (const Sample value : out)
        CHECK(value == 0.0f);

    // The read told the stream where playback went; one service pass moves
    // the window there.
    stream.service();

    stream.read(6000, 128, 0, out.data());
    CHECK(stream.underrunCount() == 1);   // no new underrun
    for (FrameCount frame = 0; frame < 128; ++frame)
        REQUIRE(out[static_cast<std::size_t>(frame)] == tone(6000 + frame));
}

TEST_CASE("a starved stream serves counted silence and never blocks")
{
    ScratchDir scratch{"incdaw-stream-starved"};

    const auto data = makeAudio(1, 4000);
    REQUIRE(bool(WavFile::write(scratch.path / "starve.wav", data)));

    AudioStream stream;
    REQUIRE(bool(stream.open(scratch.path / "starve.wav", 256)));
    stream.prefill(0);

    std::vector<Sample> out(128);

    // The prefilled window covers 512 frames; reading past it with no
    // servicing must degrade to silence, not to a wait.
    FrameCount silentFrames = 0;
    for (FrameCount position = 0; position < 2048; position += 128) {
        stream.read(position, 128, 0, out.data());

        for (const Sample value : out)
            silentFrames += value == 0.0f ? 1 : 0;
    }

    CHECK(stream.underrunCount() > 0);
    CHECK(silentFrames >= 2048 - 512);
}

TEST_CASE("the streamed read path is allocation-free")
{
    if (!rt::guardEnabled()) {
        MESSAGE("realtime guard not compiled in; not verified");
        return;
    }

    ScratchDir scratch{"incdaw-stream-rtsafe"};

    const auto data = makeAudio(2, 4000);
    REQUIRE(bool(WavFile::write(scratch.path / "rt.wav", data)));

    AudioStream stream;
    REQUIRE(bool(stream.open(scratch.path / "rt.wav", 1024)));
    stream.prefill(0);

    std::vector<Sample> out(256);

    rt::resetViolations();

    {
        const rt::ScopedRealtimeContext realtimeScope;

        for (FrameCount position = 0; position < 4000; position += 256)
            stream.read(position, 256, 0, out.data());   // includes underruns: still no allocation
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
}

// ── Through the compiler ─────────────────────────────────────────────────────

namespace {

std::vector<Sample> renderArrangement(const project::Project& projectModel,
                                      const project::GraphCompileOptions& options,
                                      FrameCount blockSize, int blocks,
                                      DiskStreamer* streamer)
{
    auto compiled = project::compileProjectGraph(projectModel, projectModel.tempoMap(), options);
    REQUIRE(bool(compiled));
    REQUIRE(compiled.warnings.empty());

    AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    std::vector<Sample> out;

    for (int block = 0; block < blocks; ++block) {
        pool.buffer(0).clear();
        compiled.graph->process(pool.buffer(0), blockSize,
                                static_cast<FramePosition>(block) * blockSize);

        const Sample* channel = pool.buffer(0).channel(0);
        for (FrameCount frame = 0; frame < blockSize; ++frame)
            out.push_back(channel[frame]);

        // Drive the service deterministically, as the background thread would.
        if (streamer != nullptr)
            streamer->serviceOnce();
    }

    return out;
}

} // namespace

TEST_CASE("a streamed clip plays identically to the same clip preloaded")
{
    ScratchDir scratch{"incdaw-stream-compile"};

    constexpr FrameCount assetFrames = 60000;
    const auto data = makeAudio(1, assetFrames);
    REQUIRE(bool(WavFile::write(scratch.path / "long.wav", data)));

    project::Project projectModel;
    auto& track = projectModel.addTrack(project::TrackType::audio, "Audio 1");
    auto& asset = projectModel.addAudioAsset((scratch.path / "long.wav").string());
    asset.sampleRate = 48000.0;
    asset.frameCount = assetFrames;

    auto& clip  = projectModel.addClip(project::ClipType::audio, track.id, asset.id);
    clip.start        = 64;
    clip.length       = 40000;
    clip.sourceOffset = 1000;

    project::GraphCompileOptions options;
    options.source     = project::PlaybackSource::arrangement;
    options.masterGain = 1.0f;

    // Preloaded reference: no streamer, so the threshold cannot trigger.
    const auto preloaded = renderArrangement(projectModel, options, 512, 80, nullptr);

    // Streamed: the threshold forces this asset onto the streamer.
    DiskStreamer streamer;
    options.diskStreamer             = &streamer;
    options.streamingThresholdFrames = 1000;

    const auto streamed = renderArrangement(projectModel, options, 512, 80, &streamer);

    REQUIRE(streamed.size() == preloaded.size());

    for (std::size_t index = 0; index < streamed.size(); ++index)
        REQUIRE(streamed[index] == preloaded[index]);
}
