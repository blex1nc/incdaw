// TRACK B (B1) — the clip properties the model stored and nothing could set.
//
// `Clip::pan` has been serialized and round-tripped since the format's first
// version, but no command wrote it and no node read it: a project could carry
// a panned clip that played dead centre. These tests pin the whole path — the
// command and its undo, the pan law resolved at compile time, and the node
// applying it — and the reference gains are computed here from the definition
// of constant power rather than by calling the class under test.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ClipCommands.h"
#include "engine/audio/AudioClipNode.h"
#include "engine/core/AudioBufferPool.h"
#include "project/Model.h"
#include "project/ProjectFile.h"
#include "project/ProjectGraphCompiler.h"

#include <atomic>
#include <cmath>
#include <filesystem>
#include <memory>
#include <vector>

using namespace incdaw;
using engine::FrameCount;
using engine::FramePosition;
using engine::Sample;

namespace fs = std::filesystem;

namespace {

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path() / ("incdaw-clipprop-" + name + "-"
                                            + std::to_string(nextSerial())))
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
        static std::atomic<int> counter{0};
        return ++counter;
    }
};

/// The constant-power pair, re-referenced so centre is unity — written from
/// the definition rather than taken from MixerStripNode, so a change to the
/// law has to be a deliberate change to this expectation too.
void referencePanGains(double pan, double& left, double& right)
{
    const double angle  = (pan + 1.0) * 0.25 * 3.14159265358979323846;
    const double centre = std::cos(0.25 * 3.14159265358979323846);

    left  = std::cos(angle) / centre;
    right = std::sin(angle) / centre;
}

Sample tone(FrameCount frame)
{
    return static_cast<Sample>(0.25 + 0.5 * std::sin(0.05 * static_cast<double>(frame)));
}

std::shared_ptr<engine::AudioFileData> makeAudio(std::size_t channels, FrameCount frames)
{
    auto data = std::make_shared<engine::AudioFileData>();
    data->sampleRate   = 48000.0;
    data->channelCount = channels;
    data->frameCount   = frames;
    data->channels.assign(channels, std::vector<Sample>(static_cast<std::size_t>(frames)));

    for (std::size_t channel = 0; channel < channels; ++channel)
        for (FrameCount frame = 0; frame < frames; ++frame)
            data->channels[channel][static_cast<std::size_t>(frame)] = tone(frame);

    return data;
}

/// One node over a range of blocks, returning both output channels.
std::vector<std::vector<Sample>> renderNode(engine::Node& node, FrameCount blockSize, int blocks)
{
    engine::AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    std::vector<std::vector<Sample>> out(2);

    for (int block = 0; block < blocks; ++block) {
        pool.buffer(0).clear();

        engine::ProcessContext context;
        context.output       = pool.buffer(0);
        context.frameCount   = blockSize;
        context.sampleRate   = 48000.0;
        context.playPosition = static_cast<FramePosition>(block) * blockSize;
        context.playing      = true;

        node.process(context);

        for (std::size_t channel = 0; channel < 2; ++channel) {
            const Sample* samples = pool.buffer(0).channel(channel);
            for (FrameCount frame = 0; frame < blockSize; ++frame)
                out[channel].push_back(samples[frame]);
        }
    }

    return out;
}

/// A project with one audio track and one clip over an in-memory asset.
struct AudioFixture {
    project::Project  project;
    project::EntityId track;
    project::EntityId clip;

    AudioFixture()
    {
        project.tempoMap().setSampleRate(48000.0);

        track = project.addTrack(project::TrackType::audio, "Audio").id;

        auto& asset      = project.addAudioAsset("/tmp/incdaw-clipprop-fake.wav");
        asset.sampleRate = 48000.0;
        asset.frameCount = 4800;

        auto& placed  = project.addClip(project::ClipType::audio, track, asset.id);
        placed.start  = 0;
        placed.length = 2400;
        clip          = placed.id;
    }

    [[nodiscard]] const project::Clip& at() const { return *project.findClip(clip); }
};

} // namespace

// ── The node ─────────────────────────────────────────────────────────────────

TEST_CASE("a panned clip is attenuated on one side and lifted on the other")
{
    double left = 0.0;
    double right = 0.0;
    referencePanGains(0.5, left, right);

    engine::AudioClipNode node;

    engine::AudioClipNode::PlacedClip clip;
    clip.audio    = makeAudio(1, 200);
    clip.start    = 0;
    clip.length   = 200;
    clip.panLeft  = static_cast<Sample>(left);
    clip.panRight = static_cast<Sample>(right);
    node.addClip(std::move(clip));

    const auto out = renderNode(node, 64, 4);

    for (FrameCount frame = 0; frame < 200; ++frame) {
        const auto index = static_cast<std::size_t>(frame);
        const double source = static_cast<double>(tone(frame));
        CHECK(static_cast<double>(out[0][index]) == doctest::Approx(source * left));
        CHECK(static_cast<double>(out[1][index]) == doctest::Approx(source * right));
    }

    // Constant power is the point of the law: the summed power is unchanged.
    CHECK(left * left + right * right == doctest::Approx(2.0));
}

TEST_CASE("an unpanned clip is bit-identical to one with no pan at all")
{
    engine::AudioClipNode panned;
    engine::AudioClipNode plain;

    double left = 0.0;
    double right = 0.0;
    referencePanGains(0.0, left, right);

    engine::AudioClipNode::PlacedClip centred;
    centred.audio    = makeAudio(2, 200);
    centred.start    = 0;
    centred.length   = 200;
    centred.panLeft  = static_cast<Sample>(left);
    centred.panRight = static_cast<Sample>(right);
    panned.addClip(std::move(centred));

    engine::AudioClipNode::PlacedClip untouched;
    untouched.audio  = makeAudio(2, 200);
    untouched.start  = 0;
    untouched.length = 200;
    plain.addClip(std::move(untouched));

    const auto withPan = renderNode(panned, 64, 4);
    const auto without = renderNode(plain, 64, 4);

    CHECK(withPan == without);
}

// ── The command ──────────────────────────────────────────────────────────────

TEST_CASE("clip pan is one undoable command over a whole selection")
{
    AudioFixture fixture;

    auto& second = fixture.project.addClip(project::ClipType::audio, fixture.track,
                                           fixture.at().source);
    second.start  = 4800;
    second.length = 2400;

    app::CommandRegistry registry{fixture.project};
    const app::ClipIds selection{fixture.clip, second.id};

    REQUIRE(registry.execute(std::make_unique<app::SetClipPanCommand>(selection, -0.75)));
    CHECK(fixture.at().pan == doctest::Approx(-0.75));
    CHECK(fixture.project.findClip(second.id)->pan == doctest::Approx(-0.75));

    REQUIRE(registry.undo());
    CHECK(fixture.at().pan == doctest::Approx(0.0));
    CHECK(fixture.project.findClip(second.id)->pan == doctest::Approx(0.0));

    REQUIRE(registry.redo());
    CHECK(fixture.at().pan == doctest::Approx(-0.75));
}

TEST_CASE("clip pan clamps, and setting the value it already has is not an undo entry")
{
    AudioFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::SetClipPanCommand>(
        app::ClipIds{fixture.clip}, 4.0)));
    CHECK(fixture.at().pan == doctest::Approx(1.0));

    CHECK_FALSE(registry.execute(std::make_unique<app::SetClipPanCommand>(
        app::ClipIds{fixture.clip}, 1.0)));
}

TEST_CASE("a pan drag merges into a single undo entry")
{
    AudioFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const app::ClipIds selection{fixture.clip};

    REQUIRE(registry.executeMerging(std::make_unique<app::SetClipPanCommand>(selection, 0.2)));
    REQUIRE(registry.executeMerging(std::make_unique<app::SetClipPanCommand>(selection, 0.6)));
    REQUIRE(registry.executeMerging(std::make_unique<app::SetClipPanCommand>(selection, 0.9)));

    CHECK(fixture.at().pan == doctest::Approx(0.9));

    REQUIRE(registry.undo());
    CHECK(fixture.at().pan == doctest::Approx(0.0));   // the whole gesture, at once
    CHECK_FALSE(registry.undo());
}

// ── Through the compiler ─────────────────────────────────────────────────────

TEST_CASE("clip pan reaches the audio through the compiled graph")
{
    ScratchDirectory scratch{"compile"};

    constexpr FrameCount assetFrames = 4800;
    REQUIRE(bool(engine::WavFile::write(scratch.path / "tone.wav", *makeAudio(1, assetFrames))));

    project::Project projectModel;
    projectModel.tempoMap().setSampleRate(48000.0);

    auto& track      = projectModel.addTrack(project::TrackType::audio, "Audio");
    auto& asset      = projectModel.addAudioAsset((scratch.path / "tone.wav").string());
    asset.sampleRate = 48000.0;
    asset.frameCount = assetFrames;

    auto& clip  = projectModel.addClip(project::ClipType::audio, track.id, asset.id);
    clip.start  = 0;
    clip.length = 2400;

    project::GraphCompileOptions options;
    options.source     = project::PlaybackSource::arrangement;
    options.masterGain = 1.0f;

    const auto render = [&]() {
        auto compiled = project::compileProjectGraph(projectModel, projectModel.tempoMap(),
                                                     options);
        REQUIRE(bool(compiled));

        engine::AudioBufferPool pool;
        pool.allocate(1, 2, 512);

        std::vector<std::vector<Sample>> out(2);

        for (int block = 0; block < 5; ++block) {
            pool.buffer(0).clear();
            compiled.graph->process(pool.buffer(0), 512,
                                    static_cast<FramePosition>(block) * 512);

            for (std::size_t channel = 0; channel < 2; ++channel) {
                const Sample* samples = pool.buffer(0).channel(channel);
                for (FrameCount frame = 0; frame < 512; ++frame)
                    out[channel].push_back(samples[frame]);
            }
        }

        return out;
    };

    const auto centred = render();

    projectModel.findClip(clip.id)->pan = 0.8;
    const auto panned = render();

    double left = 0.0;
    double right = 0.0;
    referencePanGains(0.8, left, right);

    // The strips between the clip and the output treat both channels alike, so
    // the ratio to the unpanned render is the pan law and nothing else.
    bool sawSignal = false;
    for (std::size_t index = 0; index < centred[0].size(); ++index) {
        if (std::abs(static_cast<double>(centred[0][index])) < 1.0e-4)
            continue;

        sawSignal = true;
        CHECK(static_cast<double>(panned[0][index])
              == doctest::Approx(static_cast<double>(centred[0][index]) * left).epsilon(0.01));
        CHECK(static_cast<double>(panned[1][index])
              == doctest::Approx(static_cast<double>(centred[1][index]) * right).epsilon(0.01));
    }

    CHECK(sawSignal);
}

// ── Through the project file ─────────────────────────────────────────────────

TEST_CASE("clip pan survives a save and load")
{
    ScratchDirectory scratch{"roundtrip"};

    AudioFixture fixture;
    fixture.project.findClip(fixture.clip)->pan = -0.4;

    REQUIRE(bool(project::ProjectFile::save(fixture.project, scratch.path / "p.incdaw")));

    project::Project reloaded;
    REQUIRE(bool(project::ProjectFile::load(reloaded, scratch.path / "p.incdaw")));

    const project::Clip* clip = reloaded.findClip(fixture.clip);
    REQUIRE(clip != nullptr);
    CHECK(clip->pan == doctest::Approx(-0.4));
}
