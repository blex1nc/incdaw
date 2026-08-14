// Phase 12 (part 3) — a take lands in the project, and audio clips play.
//
// The load-bearing chain: the timeline anchor maps a capture host time to a
// timeline frame; the command turns a placement into asset + clip (undoably);
// the compiler turns the clip into an AudioClipNode; and the node plays it at
// exactly the frame the model names. Each link is asserted here; the whole
// chain runs live in the app and in audiocheck.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/RecordingCommands.h"
#include "engine/AudioEngine.h"
#include "engine/audio/AudioClipNode.h"
#include "engine/core/AudioBufferPool.h"
#include "project/Model.h"
#include "project/ProjectFile.h"
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

Sample tone(FrameCount frame)
{
    return static_cast<Sample>(0.25 + 0.5 * std::sin(0.05 * static_cast<double>(frame)));
}

std::shared_ptr<AudioFileData> makeAudio(std::size_t channels, FrameCount frames,
                                         double sampleRate = 48000.0)
{
    auto data = std::make_shared<AudioFileData>();
    data->sampleRate   = sampleRate;
    data->channelCount = channels;
    data->frameCount   = frames;
    data->channels.assign(channels, std::vector<Sample>(static_cast<std::size_t>(frames)));

    for (std::size_t channel = 0; channel < channels; ++channel)
        for (FrameCount frame = 0; frame < frames; ++frame)
            data->channels[channel][static_cast<std::size_t>(frame)] = tone(frame);

    return data;
}

/// Runs one node over a range of blocks and returns channel `channel`.
std::vector<Sample> renderNode(Node& node, FrameCount blockSize, int blocks,
                               std::size_t channels = 2, std::size_t channel = 0)
{
    AudioBufferPool pool;
    pool.allocate(1, channels, blockSize);

    std::vector<Sample> samples;

    for (int block = 0; block < blocks; ++block) {
        pool.buffer(0).clear();

        ProcessContext context;
        context.output       = pool.buffer(0);
        context.frameCount   = blockSize;
        context.sampleRate   = 48000.0;
        context.playPosition = static_cast<FramePosition>(block) * blockSize;

        node.process(context);

        const Sample* out = pool.buffer(0).channel(channel);
        for (FrameCount frame = 0; frame < blockSize; ++frame)
            samples.push_back(out[frame]);
    }

    return samples;
}

} // namespace

// ── The anchor ───────────────────────────────────────────────────────────────

TEST_CASE("timeline anchor maps host times to frames, both directions in time")
{
    TimelineAnchor anchor;
    anchor.hostTimeNanos = 2'000'000'000'000ull;
    anchor.timelineFrame = 96000;
    anchor.sampleRate    = 48000.0;
    anchor.playing       = true;

    // 1 second later on the host clock is 48,000 frames later on the timeline.
    CHECK(anchor.frameAt(anchor.hostTimeNanos + 1'000'000'000ull) == 96000 + 48000);

    // Earlier host times extrapolate backwards — a take always starts before
    // the anchor block that places it.
    CHECK(anchor.frameAt(anchor.hostTimeNanos - 500'000'000ull) == 96000 - 24000);

    // One frame is 20833.3 ns at 48 kHz; rounding must land on the nearest
    // frame, not truncate.
    CHECK(anchor.frameAt(anchor.hostTimeNanos + 20833ull) == 96001);
}

// ── The node ─────────────────────────────────────────────────────────────────

TEST_CASE("audio clip plays at exactly its timeline frame")
{
    AudioClipNode node;

    AudioClipNode::PlacedClip clip;
    clip.audio  = makeAudio(1, 300);
    clip.start  = 130;             // deliberately mid-block
    clip.length = 300;

    node.addClip(std::move(clip));

    const auto out = renderNode(node, 64, 8);   // 512 frames

    for (FrameCount frame = 0; frame < 512; ++frame) {
        const Sample expected = frame >= 130 && frame < 430 ? tone(frame - 130) : 0.0f;
        REQUIRE(out[static_cast<std::size_t>(frame)] == expected);
    }
}

TEST_CASE("clip gain, source offset and mute are honoured")
{
    AudioClipNode node;

    AudioClipNode::PlacedClip loud;
    loud.audio        = makeAudio(1, 200);
    loud.start        = 0;
    loud.length       = 100;
    loud.sourceOffset = 50;        // plays source frames 50..149
    loud.gain         = 0.5f;
    node.addClip(std::move(loud));

    AudioClipNode::PlacedClip silent;
    silent.audio  = makeAudio(1, 200);
    silent.start  = 0;
    silent.length = 200;
    silent.muted  = true;
    node.addClip(std::move(silent));

    const auto out = renderNode(node, 64, 2);

    REQUIRE(out[0] == tone(50) * 0.5f);
    REQUIRE(out[99] == tone(149) * 0.5f);
    REQUIRE(out[100] == 0.0f);     // past the clip; the muted one contributed nothing
}

TEST_CASE("fades ramp linearly and a mono clip fills every output channel")
{
    AudioClipNode node;

    AudioClipNode::PlacedClip clip;
    clip.audio         = makeAudio(1, 100);
    clip.start         = 0;
    clip.length        = 100;
    clip.fadeInFrames  = 10;
    clip.fadeOutFrames = 10;
    node.addClip(std::move(clip));

    const auto left  = renderNode(node, 100, 1, 2, 0);
    const auto right = renderNode(node, 100, 1, 2, 1);

    REQUIRE(left[0] == 0.0f);                              // fade starts at zero
    REQUIRE(left[5] == tone(5) * 0.5f);                    // halfway up
    REQUIRE(left[50] == tone(50));                         // untouched middle
    REQUIRE(left[95] == tone(95) * 0.5f);                  // halfway down
    REQUIRE(left[99] == tone(99) * 0.1f);                  // last frame, nearly out

    for (std::size_t frame = 0; frame < 100; ++frame)
        REQUIRE(left[frame] == right[frame]);              // mono duplicated
}

// ── The compiler ─────────────────────────────────────────────────────────────

TEST_CASE("an audio clip in the arrangement is audible through the master")
{
    ScratchDir scratch{"incdaw-clip-compile"};

    // A real file on disk, exactly as a recorded take leaves one.
    const auto audio = makeAudio(1, 256);
    REQUIRE(bool(WavFile::write(scratch.path / "take.wav", *audio)));

    project::Project project;

    auto& track = project.addTrack(project::TrackType::audio, "Audio 1");
    auto& asset = project.addAudioAsset((scratch.path / "take.wav").string());
    asset.sampleRate   = 48000.0;
    asset.frameCount   = 256;
    asset.channelCount = 1;

    auto& clip  = project.addClip(project::ClipType::audio, track.id, asset.id);
    clip.start  = 100;
    clip.length = 256;

    project::GraphCompileOptions options;
    options.source     = project::PlaybackSource::arrangement;
    options.masterGain = 1.0f;

    auto compiled = compileProjectGraph(project, project.tempoMap(), options);
    REQUIRE(bool(compiled));
    CHECK(compiled.warnings.empty());

    AudioBufferPool pool;
    pool.allocate(1, 2, 128);

    std::vector<Sample> out;
    for (int block = 0; block < 4; ++block) {
        pool.buffer(0).clear();
        compiled.graph->process(pool.buffer(0), 128, static_cast<FramePosition>(block) * 128);

        const Sample* channel = pool.buffer(0).channel(0);
        for (FrameCount frame = 0; frame < 128; ++frame)
            out.push_back(channel[frame]);
    }

    // The master strip pans centre with constant power: both channels carry
    // the clip scaled by 1/sqrt(2). What matters here is placement.
    const Sample panScale = out[100] / tone(0);
    REQUIRE(panScale > 0.5f);

    for (FrameCount frame = 0; frame < 512; ++frame) {
        const Sample expected = frame >= 100 && frame < 356 ? tone(frame - 100) * panScale : 0.0f;
        REQUIRE(std::abs(out[static_cast<std::size_t>(frame)] - expected) < 1.0e-6f);
    }
}

TEST_CASE("clip normalize scales the clip's content to full scale, pre-mixer")
{
    ScratchDir scratch{"incdaw-clip-normalize"};

    const auto audio = makeAudio(1, 256);   // tone() peaks well below 1.0
    REQUIRE(bool(WavFile::write(scratch.path / "quiet.wav", *audio)));

    Sample peak = 0.0f;
    for (const Sample value : audio->channels[0])
        peak = std::max(peak, std::abs(value));
    REQUIRE(peak < 0.9f);

    project::Project projectModel;
    auto& track = projectModel.addTrack(project::TrackType::audio, "Audio 1");
    auto& asset = projectModel.addAudioAsset((scratch.path / "quiet.wav").string());
    asset.sampleRate = 48000.0;
    asset.frameCount = 256;

    auto& clip  = projectModel.addClip(project::ClipType::audio, track.id, asset.id);
    clip.start  = 0;
    clip.length = 256;

    project::GraphCompileOptions options;
    options.source     = project::PlaybackSource::arrangement;
    options.masterGain = 1.0f;

    const auto render = [&]() {
        auto compiled = compileProjectGraph(projectModel, projectModel.tempoMap(), options);
        REQUIRE(bool(compiled));
        CHECK(compiled.warnings.empty());

        AudioBufferPool pool;
        pool.allocate(1, 2, 256);
        pool.buffer(0).clear();
        compiled.graph->process(pool.buffer(0), 256, 0);

        std::vector<Sample> out(256);
        for (FrameCount frame = 0; frame < 256; ++frame)
            out[static_cast<std::size_t>(frame)] = pool.buffer(0).channel(0)[frame];
        return out;
    };

    const auto plain = render();

    projectModel.findClip(clip.id)->normalize = true;
    const auto normalized = render();

    // Normalize is exactly a pre-mixer gain of 1/peak — recallable from the
    // model, applied before the strip.
    for (FrameCount frame = 0; frame < 256; ++frame)
        REQUIRE(std::abs(normalized[static_cast<std::size_t>(frame)]
                         - plain[static_cast<std::size_t>(frame)] / peak) < 1.0e-5f);
}

TEST_CASE("a wrong-rate asset stays silent and says why")
{
    ScratchDir scratch{"incdaw-clip-wrongrate"};

    const auto audio = makeAudio(1, 128, 44100.0);
    REQUIRE(bool(WavFile::write(scratch.path / "cd.wav", *audio)));

    project::Project project;
    auto& track = project.addTrack(project::TrackType::audio, "Audio 1");
    auto& asset = project.addAudioAsset((scratch.path / "cd.wav").string());
    auto& clip  = project.addClip(project::ClipType::audio, track.id, asset.id);
    clip.start  = 0;
    clip.length = 128;

    project::GraphCompileOptions options;
    options.source = project::PlaybackSource::arrangement;

    auto compiled = compileProjectGraph(project, project.tempoMap(), options);
    REQUIRE(bool(compiled));
    REQUIRE(compiled.warnings.size() == 1);
    CHECK(compiled.warnings.front().find("44100") != std::string::npos);

    AudioBufferPool pool;
    pool.allocate(1, 2, 128);
    pool.buffer(0).clear();
    compiled.graph->process(pool.buffer(0), 128, 0);

    for (FrameCount frame = 0; frame < 128; ++frame)
        REQUIRE(pool.buffer(0).channel(0)[frame] == 0.0f);
}

// ── The command ──────────────────────────────────────────────────────────────

TEST_CASE("a placed take becomes track, asset and clip — and undoes as one")
{
    project::Project project;
    app::CommandRegistry registry{project};

    project::RecordingSession::Placement placement;
    placement.succeeded    = true;
    placement.path         = "/tmp/take-test.wav";
    placement.startFrame   = 4321;
    placement.frameCount   = 48000;
    placement.channelCount = 2;
    placement.sampleRate   = 48000.0;

    REQUIRE(registry.execute(std::make_unique<app::InsertRecordedTakeCommand>(placement)));

    REQUIRE(project.tracks().size() == 1);
    REQUIRE(project.tracks().front().type == project::TrackType::audio);
    REQUIRE(project.audioAssets().size() == 1);
    REQUIRE(project.clips().size() == 1);

    const auto trackId = project.tracks().front().id;
    const auto assetId = project.audioAssets().front().id;
    const auto clipId  = project.clips().front().id;

    const auto& clip = project.clips().front();
    CHECK(clip.type == project::ClipType::audio);
    CHECK(clip.track == trackId);
    CHECK(clip.source == assetId);
    CHECK(clip.start == 4321);
    CHECK(clip.length == 48000);

    CHECK(project.audioAssets().front().frameCount == 48000);

    REQUIRE(registry.undo());
    CHECK(project.tracks().empty());
    CHECK(project.audioAssets().empty());
    CHECK(project.clips().empty());

    // Redo restores the same ids, so later commands on the stack stay valid.
    REQUIRE(registry.redo());
    REQUIRE(project.tracks().size() == 1);
    CHECK(project.tracks().front().id == trackId);
    CHECK(project.audioAssets().front().id == assetId);
    CHECK(project.clips().front().id == clipId);
}

TEST_CASE("an existing audio track receives the take; none is created")
{
    project::Project project;

    // The id is copied out at once: the next addTrack may reallocate the
    // vector and a kept reference would dangle.
    const auto existingId = project.addTrack(project::TrackType::audio, "My Audio").id;
    project.addTrack(project::TrackType::instrument, "Synth");

    project::RecordingSession::Placement placement;
    placement.succeeded    = true;
    placement.path         = "/tmp/take-test2.wav";
    placement.startFrame   = 0;
    placement.frameCount   = 100;
    placement.channelCount = 1;
    placement.sampleRate   = 48000.0;

    app::InsertRecordedTakeCommand command{placement};
    REQUIRE(command.execute(project));

    REQUIRE(project.tracks().size() == 2);
    CHECK(project.clips().front().track == existingId);

    command.undo(project);
    CHECK(project.tracks().size() == 2);   // the pre-existing tracks survive undo
}

// ── Serialization ────────────────────────────────────────────────────────────

TEST_CASE("a project with a recorded take round-trips through the file format")
{
    ScratchDir scratch{"incdaw-take-serialize"};

    project::Project project;
    auto& track = project.addTrack(project::TrackType::audio, "Audio 1");
    auto& asset = project.addAudioAsset("/tmp/take-roundtrip.wav");
    asset.sampleRate   = 48000.0;
    asset.frameCount   = 12345;
    asset.channelCount = 2;

    auto& clip  = project.addClip(project::ClipType::audio, track.id, asset.id);
    clip.start        = 777;
    clip.length       = 12345;
    clip.sourceOffset = 11;
    clip.name         = "take-roundtrip";

    const auto saved = project::ProjectFile::save(project, scratch.path / "Take.incdaw");
    REQUIRE(saved.succeeded);

    project::Project loaded;
    const auto read = project::ProjectFile::load(loaded, scratch.path / "Take.incdaw");
    REQUIRE(read.succeeded);

    REQUIRE(loaded == project);
}
