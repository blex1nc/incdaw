// TRACK B (B7) — crossfades.
//
// A crossfade is the paired verb over an overlap. The command marks two edges;
// the fade LENGTHS are the overlap, worked out wherever the fades are read —
// which is the whole design, because there is then no stored length to go
// stale when either clip is moved or resized.
//
// The headline test is the brief's sentence taken literally: two clips of
// constant 1.0 crossfaded must sum to 1.0 across the overlap, and must still
// sum to 1.0 after one of them is resized. Linear complementary fades sum to
// unity exactly, so this is an equality about the audio and not a proxy for
// one.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ClipCommands.h"
#include "engine/audio/WavFile.h"
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
        : path(fs::temp_directory_path() / ("incdaw-xfade-" + name + "-"
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

/// Two audio clips of a flat 1.0 tone on one lane, the second overlapping the
/// first. Flat rather than a tone so the sum is readable at every frame.
struct CrossfadeFixture {
    ScratchDirectory  scratch{"fixture"};
    project::Project  project;
    project::EntityId track;
    project::EntityId earlier;
    project::EntityId later;

    static constexpr FrameCount assetFrames = 8000;

    CrossfadeFixture()
    {
        auto data = std::make_shared<engine::AudioFileData>();
        data->sampleRate   = 48000.0;
        data->channelCount = 1;
        data->frameCount   = assetFrames;
        data->channels.assign(1, std::vector<Sample>(assetFrames, 1.0f));

        REQUIRE(bool(engine::WavFile::write(scratch.path / "flat.wav", *data)));

        project.tempoMap().setSampleRate(48000.0);

        track            = project.addTrack(project::TrackType::audio, "Audio").id;
        auto& asset      = project.addAudioAsset((scratch.path / "flat.wav").string());
        asset.sampleRate = 48000.0;
        asset.frameCount = assetFrames;

        earlier = place(asset.id, 0, 4000);
        later   = place(asset.id, 3000, 4000);   // 1,000 frames of overlap
    }

    project::EntityId place(project::EntityId asset, FramePosition start, FrameCount length)
    {
        project::Clip& clip = project.addClip(project::ClipType::audio, track, asset);
        clip.start          = start;
        clip.length         = length;
        return clip.id;
    }

    [[nodiscard]] project::Clip& at(project::EntityId id) { return *project.findClip(id); }

    /// The compiled arrangement, channel 0.
    [[nodiscard]] std::vector<Sample> render(int blocks = 20)
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

/// The level the master strip leaves a lone 1.0 clip at, measured rather than
/// assumed, so the sum test compares audio with audio.
double soloLevel(CrossfadeFixture& fixture)
{
    const auto rendered = fixture.render(4);
    return static_cast<double>(rendered[500]);   // inside the first clip, before any overlap
}

} // namespace

// ── The brief's sentence, literally ──────────────────────────────────────────

TEST_CASE("a crossfaded pair sums to unity across the overlap, and still does after a resize")
{
    CrossfadeFixture fixture;

    const double level = soloLevel(fixture);
    REQUIRE(level > 0.0);

    // Before: the overlap is twice as loud, which is what an untouched overlap
    // of two identical clips is.
    const auto stacked = fixture.render();
    CHECK(static_cast<double>(stacked[3500]) == doctest::Approx(level * 2.0).epsilon(0.01));

    app::CommandRegistry registry{fixture.project};
    REQUIRE(registry.execute(std::make_unique<app::CrossfadeClipsCommand>(
        app::ClipIds{fixture.earlier, fixture.later})));

    const auto faded = fixture.render();

    // Across the whole 1,000-frame overlap, and at both of its edges.
    for (FrameCount frame = 3010; frame < 3990; frame += 70)
        CHECK(static_cast<double>(faded[static_cast<std::size_t>(frame)])
              == doctest::Approx(level).epsilon(0.02));

    // Now lengthen the earlier clip. A sixteenth at 120 bpm is 1,500 frames,
    // which keeps both clips inside the 8,000 frames of audio they play — a
    // longer stretch would run the earlier one off the end of its asset and
    // the test would be measuring silence rather than a fade.
    REQUIRE(registry.execute(std::make_unique<app::ResizeClipsCommand>(
        app::ClipIds{fixture.earlier}, engine::ticksPerQuarterNote / 16)));

    const FramePosition end = fixture.at(fixture.earlier).start
                            + static_cast<FramePosition>(fixture.at(fixture.earlier).length);
    REQUIRE(end > 4000);
    REQUIRE(end < static_cast<FramePosition>(CrossfadeFixture::assetFrames));

    // The overlap grew with the clip, and the fades grew with the overlap.
    CHECK(project::clipFades(fixture.project, fixture.at(fixture.earlier)).out
          == static_cast<FrameCount>(end - 3000));

    const auto resized = fixture.render();

    for (FramePosition frame = 3050; frame < end - 50; frame += 100)
        CHECK(static_cast<double>(resized[static_cast<std::size_t>(frame)])
              == doctest::Approx(level).epsilon(0.02));
}

// ── The derivation ───────────────────────────────────────────────────────────

TEST_CASE("the fade lengths are the overlap, and follow it")
{
    CrossfadeFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::CrossfadeClipsCommand>(
        app::ClipIds{fixture.earlier, fixture.later})));

    CHECK(project::clipFades(fixture.project, fixture.at(fixture.earlier)).out == 1000);
    CHECK(project::clipFades(fixture.project, fixture.at(fixture.later)).in == 1000);

    // Nothing was written to the clips themselves.
    CHECK(fixture.at(fixture.earlier).fadeOutFrames == 0);
    CHECK(fixture.at(fixture.later).fadeInFrames == 0);

    // Slide the later clip: the overlap halves, and so do both fades.
    fixture.at(fixture.later).start = 3500;

    CHECK(project::clipFades(fixture.project, fixture.at(fixture.earlier)).out == 500);
    CHECK(project::clipFades(fixture.project, fixture.at(fixture.later)).in == 500);

    // Slide it clear: no overlap, no crossfade, and no stale length left over.
    fixture.at(fixture.later).start = 9000;

    CHECK(project::clipFades(fixture.project, fixture.at(fixture.earlier)).out == 0);
    CHECK(project::clipFades(fixture.project, fixture.at(fixture.later)).in == 0);
}

TEST_CASE("a crossfade overrides only the edge it is on")
{
    CrossfadeFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::SetClipFadesCommand>(
        app::ClipIds{fixture.earlier}, 250, 250)));

    REQUIRE(registry.execute(std::make_unique<app::CrossfadeClipsCommand>(
        app::ClipIds{fixture.earlier, fixture.later})));

    const project::ClipFades fades = project::clipFades(fixture.project,
                                                        fixture.at(fixture.earlier));
    CHECK(fades.out == 1000);   // the crossfade
    CHECK(fades.in == 250);     // the manual fade, untouched
}

TEST_CASE("three clips on a lane keep two separate crossfades, and lose one at a time")
{
    CrossfadeFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId last =
        fixture.place(fixture.at(fixture.later).source, 6000, 4000);

    const app::ClipIds all{fixture.earlier, fixture.later, last};

    REQUIRE(registry.execute(std::make_unique<app::CrossfadeClipsCommand>(all)));

    CHECK(fixture.at(fixture.earlier).crossfadeOut);
    CHECK(fixture.at(fixture.later).crossfadeIn);
    CHECK(fixture.at(fixture.later).crossfadeOut);
    CHECK(fixture.at(last).crossfadeIn);

    // Removing the second pair leaves the first standing — which is the case a
    // single per-clip flag could not express.
    REQUIRE(registry.execute(std::make_unique<app::CrossfadeClipsCommand>(
        app::ClipIds{fixture.later, last}, false)));

    CHECK(fixture.at(fixture.earlier).crossfadeOut);
    CHECK(fixture.at(fixture.later).crossfadeIn);
    CHECK_FALSE(fixture.at(fixture.later).crossfadeOut);
    CHECK_FALSE(fixture.at(last).crossfadeIn);

    CHECK(project::clipFades(fixture.project, fixture.at(fixture.later)).in == 1000);
    CHECK(project::clipFades(fixture.project, fixture.at(fixture.later)).out == 0);
}

// ── Refusals and undo ────────────────────────────────────────────────────────

TEST_CASE("a crossfade needs two overlapping audio clips on one lane")
{
    CrossfadeFixture fixture;
    app::CommandRegistry registry{fixture.project};

    // Not overlapping.
    fixture.at(fixture.later).start = 9000;
    CHECK_FALSE(registry.execute(std::make_unique<app::CrossfadeClipsCommand>(
        app::ClipIds{fixture.earlier, fixture.later})));

    // Overlapping, but on different lanes — lanes are what stop them fighting
    // in the first place.
    fixture.at(fixture.later).start = 3000;
    fixture.at(fixture.later).lane  = 1;
    CHECK_FALSE(registry.execute(std::make_unique<app::CrossfadeClipsCommand>(
        app::ClipIds{fixture.earlier, fixture.later})));

    // One clip on its own.
    fixture.at(fixture.later).lane = 0;
    CHECK_FALSE(registry.execute(std::make_unique<app::CrossfadeClipsCommand>(
        app::ClipIds{fixture.earlier})));
}

TEST_CASE("crossfading is one undo entry, and a locked clip still takes it")
{
    CrossfadeFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::SetClipLockedCommand>(
        app::ClipIds{fixture.earlier, fixture.later}, true)));

    // A lock protects an arrangement decision; a fade is a mix one.
    REQUIRE(registry.execute(std::make_unique<app::CrossfadeClipsCommand>(
        app::ClipIds{fixture.earlier, fixture.later})));

    REQUIRE(registry.undo());
    CHECK_FALSE(fixture.at(fixture.earlier).crossfadeOut);
    CHECK_FALSE(fixture.at(fixture.later).crossfadeIn);

    REQUIRE(registry.redo());
    CHECK(fixture.at(fixture.earlier).crossfadeOut);
}

TEST_CASE("clip fades are undoable, clamped to the clip, and merge into one drag")
{
    CrossfadeFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const app::ClipIds one{fixture.earlier};

    REQUIRE(registry.executeMerging(std::make_unique<app::SetClipFadesCommand>(one, 100, -1)));
    REQUIRE(registry.executeMerging(std::make_unique<app::SetClipFadesCommand>(one, 300, -1)));

    CHECK(fixture.at(fixture.earlier).fadeInFrames == 300);
    CHECK(fixture.at(fixture.earlier).fadeOutFrames == 0);

    REQUIRE(registry.undo());
    CHECK(fixture.at(fixture.earlier).fadeInFrames == 0);
    CHECK_FALSE(registry.undo());

    // A fade longer than the clip would ramp past its own end.
    REQUIRE(registry.execute(std::make_unique<app::SetClipFadesCommand>(one, 999999, -1)));
    CHECK(fixture.at(fixture.earlier).fadeInFrames == fixture.at(fixture.earlier).length);
}

// ── Through the project file ─────────────────────────────────────────────────

TEST_CASE("crossfade flags round-trip, and the fades come back derived")
{
    ScratchDirectory scratch{"roundtrip"};

    CrossfadeFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::CrossfadeClipsCommand>(
        app::ClipIds{fixture.earlier, fixture.later})));

    const fs::path file = scratch.path / "Crossfade.incdaw";
    REQUIRE(bool(project::ProjectFile::save(fixture.project, file)));

    project::Project reloaded;
    REQUIRE(bool(project::ProjectFile::load(reloaded, file)));

    const project::Clip* earlier = reloaded.findClip(fixture.earlier);
    REQUIRE(earlier != nullptr);
    CHECK(earlier->crossfadeOut);
    CHECK(earlier->fadeOutFrames == 0);   // still nothing stored
    CHECK(project::clipFades(reloaded, *earlier).out == 1000);
}

TEST_CASE("the v1.10 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.10" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    project::Project project;
    const auto result = project::ProjectFile::load(project, fixture);
    REQUIRE(result.succeeded);

    CHECK(project.metadata().title == "Format v1.10 fixture");

    REQUIRE(project.clips().size() == 2);
    CHECK(project.clips()[0].crossfadeOut);
    CHECK(project.clips()[1].crossfadeIn);
    CHECK(project::clipFades(project, project.clips()[0]).out == 1000);
}
