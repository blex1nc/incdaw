// TRACK B (B8) — consolidation.
//
// The load-bearing property is that consolidating changes nothing you can
// hear: render the arrangement, consolidate a track's clips, render it again,
// and the two must match sample for sample within the error a float round trip
// through a WAV can introduce. That is only true because the render runs on a
// COPY whose mixer nodes are flat — a consolidation that baked the strip in
// would then play the strip twice, and the test below would catch it.
//
// Nothing in OfflineRender is touched; the command hands it a stripped project
// and a region.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ClipCommands.h"
#include "app/commands/ConsolidateCommands.h"
#include "engine/audio/WavFile.h"
#include "engine/core/AudioBufferPool.h"
#include "project/Model.h"
#include "project/OfflineRender.h"
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
        : path(fs::temp_directory_path() / ("incdaw-consolidate-" + name + "-"
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

Sample tone(FrameCount frame, double step)
{
    return static_cast<Sample>(0.4 * std::sin(step * static_cast<double>(frame)));
}

/// Two audio clips of different material, back to back on one track, plus a
/// mixer strip that is deliberately NOT at unity.
struct ConsolidateFixture {
    ScratchDirectory  scratch{"fixture"};
    project::Project  project;
    project::EntityId track;
    project::EntityId first;
    project::EntityId second;

    static constexpr FrameCount assetFrames = 6000;

    ConsolidateFixture()
    {
        project.tempoMap().setSampleRate(48000.0);

        track = project.addTrack(project::TrackType::audio, "Audio").id;

        first  = place(write("a.wav", 0.037), 0, 3000);
        second = place(write("b.wav", 0.091), 3000, 3000);

        // A strip that colours the sound, so a consolidation that baked it in
        // would be audible immediately.
        project::MixerNode* master = project.findMixerNode(project.masterMixerNode());
        REQUIRE(master != nullptr);
        master->volume = 0.6;
        master->pan    = -0.3;
    }

    project::EntityId write(const char* name, double step)
    {
        auto data = std::make_shared<engine::AudioFileData>();
        data->sampleRate   = 48000.0;
        data->channelCount = 1;
        data->frameCount   = assetFrames;
        data->channels.assign(1, std::vector<Sample>(assetFrames));

        for (FrameCount frame = 0; frame < assetFrames; ++frame)
            data->channels[0][static_cast<std::size_t>(frame)] = tone(frame, step);

        REQUIRE(bool(engine::WavFile::write(scratch.path / name, *data)));

        auto& asset      = project.addAudioAsset((scratch.path / name).string());
        asset.sampleRate = 48000.0;
        asset.frameCount = assetFrames;
        return asset.id;
    }

    project::EntityId place(project::EntityId asset, FramePosition start, FrameCount length)
    {
        project::Clip& clip = project.addClip(project::ClipType::audio, track, asset);
        clip.start          = start;
        clip.length         = length;
        return clip.id;
    }

    [[nodiscard]] project::Clip& at(project::EntityId id) { return *project.findClip(id); }

    [[nodiscard]] std::vector<Sample> render(int blocks = 16)
    {
        project::GraphCompileOptions options;
        options.source     = project::PlaybackSource::arrangement;
        options.masterGain = 1.0f;

        auto compiled = project::compileProjectGraph(project, project.tempoMap(), options);
        REQUIRE(bool(compiled));

        engine::AudioBufferPool pool;
        pool.allocate(1, 2, 512);

        std::vector<Sample> out;

        for (int block = 0; block < blocks; ++block) {
            pool.buffer(0).clear();
            compiled.graph->process(pool.buffer(0), 512,
                                    static_cast<FramePosition>(block) * 512);

            const Sample* samples = pool.buffer(0).channel(0);
            for (FrameCount frame = 0; frame < 512; ++frame)
                out.push_back(samples[frame]);
        }

        return out;
    }
};

} // namespace

// ── The load-bearing property ────────────────────────────────────────────────

TEST_CASE("consolidating a track changes nothing you can hear")
{
    ScratchDirectory output{"render"};

    ConsolidateFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const auto before = fixture.render();

    REQUIRE(registry.execute(std::make_unique<app::ConsolidateClipsCommand>(
        app::ClipIds{fixture.first, fixture.second}, output.path / "one.wav")));

    // Two clips became one.
    REQUIRE(fixture.project.clips().size() == 1);
    const project::Clip& consolidated = fixture.project.clips()[0];
    CHECK(consolidated.type == project::ClipType::audio);
    CHECK(consolidated.track == fixture.track);
    CHECK(consolidated.start == 0);
    CHECK(consolidated.length == 6000);

    const auto after = fixture.render();

    for (std::size_t index = 0; index < 6000; ++index)
        REQUIRE(static_cast<double>(after[index])
                == doctest::Approx(static_cast<double>(before[index])).epsilon(0.001));
}

TEST_CASE("the source clips come back, and so does the arrangement")
{
    ScratchDirectory output{"undo"};

    ConsolidateFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const auto before = fixture.render();

    REQUIRE(registry.execute(std::make_unique<app::ConsolidateClipsCommand>(
        app::ClipIds{fixture.first, fixture.second}, output.path / "one.wav")));

    REQUIRE(registry.undo());

    REQUIRE(fixture.project.clips().size() == 2);
    CHECK(fixture.project.findClip(fixture.first) != nullptr);
    CHECK(fixture.project.findClip(fixture.second) != nullptr);

    // The asset the render created went with it — an undone consolidation
    // leaves no orphan in the project.
    CHECK(fixture.project.audioAssets().size() == 2);

    const auto restored = fixture.render();
    for (std::size_t index = 0; index < 6000; ++index)
        REQUIRE(restored[index] == before[index]);

    // The file itself is left on disk, exactly as an undone take is.
    CHECK(fs::exists(output.path / "one.wav"));

    // And redo lands on the same clip and the same asset rather than minting
    // a second pair.
    REQUIRE(registry.redo());
    CHECK(fixture.project.clips().size() == 1);
    CHECK(fixture.project.audioAssets().size() == 3);
}

// ── What it refuses ──────────────────────────────────────────────────────────

TEST_CASE("consolidation needs a selection on one track")
{
    ScratchDirectory output{"refuse"};

    ConsolidateFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId other =
        fixture.project.addTrack(project::TrackType::audio, "Other").id;

    project::Clip& stray = fixture.project.addClip(project::ClipType::audio, other,
                                                   fixture.at(fixture.first).source);
    stray.start  = 0;
    stray.length = 1000;

    const app::ClipIds across{fixture.first, stray.id};

    // The reason lives in a free function, not on the command: a command the
    // registry refuses is destroyed before the caller could ask it anything.
    CHECK(app::consolidationRefusal(fixture.project, across) == "select clips on one track");

    CHECK_FALSE(registry.execute(
        std::make_unique<app::ConsolidateClipsCommand>(across, output.path / "one.wav")));

    // Refused before anything was touched.
    CHECK(fixture.project.clips().size() == 3);
    CHECK_FALSE(fs::exists(output.path / "one.wav"));

    CHECK(app::consolidationRefusal(fixture.project, {}) == "nothing selected");
    CHECK_FALSE(registry.execute(std::make_unique<app::ConsolidateClipsCommand>(
        app::ClipIds{}, output.path / "two.wav")));
}

TEST_CASE("consolidating one clip is allowed, and lands on its lane")
{
    ScratchDirectory output{"single"};

    ConsolidateFixture fixture;
    app::CommandRegistry registry{fixture.project};

    fixture.at(fixture.second).lane = 1;

    auto command = std::make_unique<app::ConsolidateClipsCommand>(
        app::ClipIds{fixture.second}, output.path / "one.wav");
    app::ConsolidateClipsCommand* raw = command.get();

    REQUIRE(registry.execute(std::move(command)));

    const project::Clip* made = fixture.project.findClip(raw->consolidatedClip());
    REQUIRE(made != nullptr);
    CHECK(made->lane == 1);
    CHECK(made->start == 3000);
    CHECK(fixture.project.findClip(fixture.first) != nullptr);   // untouched
}

// ── A pattern track consolidates too ─────────────────────────────────────────

TEST_CASE("pattern clips consolidate into audio")
{
    ScratchDirectory output{"pattern"};

    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    const project::EntityId channel = project.addChannel("Lead").id;

    project::Pattern& pattern = project.addPattern("P1");
    pattern.length            = engine::ticksPerQuarterNote * 4;

    project::MidiEvent note;
    note.type     = project::MidiEventType::note;
    note.tick     = 0;
    note.duration = engine::ticksPerQuarterNote * 2;
    note.key      = 60;
    note.value    = 100;
    pattern.contentFor(channel).events.push_back(note);

    const project::EntityId track =
        project.addTrack(project::TrackType::instrument, "Inst").id;

    project::Clip& clip = project.addClip(project::ClipType::pattern, track, pattern.id);
    clip.startTick      = 0;
    clip.lengthTicks    = pattern.length;

    app::CommandRegistry registry{project};

    auto command = std::make_unique<app::ConsolidateClipsCommand>(
        app::ClipIds{clip.id}, output.path / "pattern.wav");
    app::ConsolidateClipsCommand* raw = command.get();

    REQUIRE(registry.execute(std::move(command)));

    const project::Clip* made = project.findClip(raw->consolidatedClip());
    REQUIRE(made != nullptr);
    CHECK(made->type == project::ClipType::audio);
    CHECK(made->track == track);

    // The render is not silence: the instrument actually played.
    engine::AudioFileData written;
    REQUIRE(bool(engine::WavFile::read(output.path / "pattern.wav", written)));
    REQUIRE(written.frameCount > 0);

    bool anySignal = false;
    for (const auto& channelData : written.channels)
        for (const Sample sample : channelData)
            anySignal = anySignal || std::abs(static_cast<double>(sample)) > 1.0e-4;

    CHECK(anySignal);
}
