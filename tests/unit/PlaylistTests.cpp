#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/PlaylistModel.h"
#include "app/commands/ArrangementCommands.h"
#include "engine/core/AudioBufferPool.h"
#include "project/GraphCompiler.h"
#include "project/PatternCompiler.h"
#include "project/ProjectFile.h"

#include <filesystem>
#include <memory>
#include <vector>

using namespace incdaw;
using engine::Tick;
using engine::ticksPerQuarterNote;
using project::Clip;
using project::EntityId;
using project::MidiEvent;
using project::Pattern;
using project::Project;

namespace fs = std::filesystem;

namespace {

/// A project with one channel, one one-bar pattern and one instrument track.
struct Arrangement {
    Project  project;
    EntityId channel;
    EntityId track;
    EntityId pattern;
    project::FrameCount barFrames = 0;

    Arrangement()
    {
        channel = project.addChannel("Lead").id;
        track   = project.addTrack(project::TrackType::instrument, "Track 1").id;

        Pattern& created = project.addPattern("Riff");
        created.length = ticksPerQuarterNote * 4;

        MidiEvent note;
        note.type      = project::MidiEventType::note;
        note.tick      = 0;
        note.duration  = ticksPerQuarterNote;
        note.key       = 60;
        note.value     = 100;
        note.channelId = channel;
        created.events.push_back(note);

        pattern   = created.id;
        barFrames = static_cast<project::FrameCount>(
            project.tempoMap().frameForTick(created.length));
    }

    EntityId place(project::FramePosition start, EntityId onTrack = {})
    {
        Clip& clip = project.addClip(project::ClipType::pattern,
                                     onTrack.isValid() ? onTrack : track, pattern);
        clip.start  = start;
        clip.length = barFrames;
        return clip.id;
    }
};

app::PlaylistModel::Viewport standardViewport(project::FrameCount visibleFrames)
{
    app::PlaylistModel::Viewport viewport;
    viewport.firstFrame    = 0;
    viewport.visibleFrames = visibleFrames;
    viewport.firstTrack    = 0;
    viewport.visibleTracks = 4;
    viewport.width         = 800.0;
    viewport.height        = 400.0;
    return viewport;
}

} // namespace

// ── Geometry ──────────────────────────────────────────────────────────────────

TEST_CASE("the playlist culls to its viewport and hit tests what it drew")
{
    Arrangement arrangement;
    const EntityId first  = arrangement.place(0);
    const EntityId second = arrangement.place(arrangement.barFrames * 2);
    const EntityId far    = arrangement.place(arrangement.barFrames * 40);

    app::PlaylistModel model;
    model.setViewport(standardViewport(arrangement.barFrames * 4));

    std::vector<app::PlaylistModel::VisibleClip> visible;
    model.collectVisibleClips(arrangement.project, visible);

    REQUIRE(visible.size() == 2);              // the far clip is off screen
    CHECK(visible[0].id == first);
    CHECK(visible[1].id == second);
    CHECK(visible[0].x == doctest::Approx(0.0));
    CHECK(visible[0].width == doctest::Approx(200.0));   // a quarter of 800 points
    CHECK(visible[1].x == doctest::Approx(400.0));

    const double row = model.trackToY(0) + model.trackHeight() * 0.5;

    CHECK(model.clipAtPoint(arrangement.project, 10.0, row) == first);
    CHECK(model.clipAtPoint(arrangement.project, 410.0, row) == second);
    CHECK_FALSE(model.clipAtPoint(arrangement.project, 300.0, row).isValid());   // the gap
    CHECK(far.isValid());

    // The right edge of a clip resizes rather than moves.
    CHECK(model.isOverResizeHandle(arrangement.project, first, 196.0, row));
    CHECK_FALSE(model.isOverResizeHandle(arrangement.project, first, 100.0, row));
}

TEST_CASE("a clip reaching into the viewport is still drawn")
{
    Arrangement arrangement;
    const EntityId clip = arrangement.place(-arrangement.barFrames / 2);

    app::PlaylistModel model;
    model.setViewport(standardViewport(arrangement.barFrames * 4));

    std::vector<app::PlaylistModel::VisibleClip> visible;
    model.collectVisibleClips(arrangement.project, visible);

    REQUIRE(visible.size() == 1);
    CHECK(visible[0].id == clip);
    CHECK(visible[0].x < 0.0);   // starts off the left edge, drawn anyway
}

TEST_CASE("snapping rounds to the nearest line, including before zero")
{
    app::PlaylistModel model;
    model.setSnap(1000);

    CHECK(model.snapFrame(0) == 0);
    CHECK(model.snapFrame(400) == 0);
    CHECK(model.snapFrame(600) == 1000);
    CHECK(model.snapFrame(1500) == 2000);
    CHECK(model.snapFrame(-400) == 0);
    CHECK(model.snapFrame(-600) == -1000);

    model.setSnap(0);
    CHECK(model.snapFrame(1234) == 1234);   // free placement
}

TEST_CASE("box selection takes the clips it covers and drops the ones that vanish")
{
    Arrangement arrangement;
    const EntityId first  = arrangement.place(0);
    const EntityId second = arrangement.place(arrangement.barFrames * 2);

    app::PlaylistModel model;
    model.setViewport(standardViewport(arrangement.barFrames * 4));

    std::vector<EntityId> selected;
    model.clipsInRectangle(arrangement.project, 0.0, 0.0, 800.0, 400.0, selected);
    CHECK(selected.size() == 2);

    model.clipsInRectangle(arrangement.project, 0.0, 0.0, 250.0, 400.0, selected);
    REQUIRE(selected.size() == 1);
    CHECK(selected[0] == first);

    model.setSelection({first, second, first});
    CHECK(model.selection().size() == 2);   // de-duplicated

    arrangement.project.clips().erase(arrangement.project.clips().begin());
    model.pruneSelection(arrangement.project);

    REQUIRE(model.selection().size() == 1);
    CHECK(model.selection()[0] == second);
}

// ── Clip editing ──────────────────────────────────────────────────────────────

TEST_CASE("splitting a clip keeps the left half's identity and offsets the right")
{
    Arrangement arrangement;
    app::CommandRegistry registry{arrangement.project};

    const EntityId clip = arrangement.place(0);
    const auto     at   = static_cast<project::FramePosition>(arrangement.barFrames / 4);

    auto split = std::make_unique<app::SplitClipCommand>(clip, at);
    app::SplitClipCommand* pointer = split.get();
    REQUIRE(registry.execute(std::move(split)));

    const Clip* left  = arrangement.project.findClip(clip);
    const Clip* right = arrangement.project.findClip(pointer->createdClip());

    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);

    CHECK(left->start == 0);
    CHECK(left->length == arrangement.barFrames / 4);
    CHECK(right->start == at);
    CHECK(right->length == arrangement.barFrames - arrangement.barFrames / 4);
    CHECK(right->sourceOffset == arrangement.barFrames / 4);
    CHECK(right->source == left->source);

    REQUIRE(registry.undo());
    CHECK(arrangement.project.clips().size() == 1);
    CHECK(arrangement.project.findClip(clip)->length == arrangement.barFrames);
}

TEST_CASE("a cut outside the clip is not an edit")
{
    Arrangement arrangement;
    app::CommandRegistry registry{arrangement.project};

    const EntityId clip = arrangement.place(arrangement.barFrames);

    CHECK_FALSE(registry.execute(std::make_unique<app::SplitClipCommand>(clip, 0)));
    CHECK_FALSE(registry.execute(std::make_unique<app::SplitClipCommand>(
        clip, static_cast<project::FramePosition>(arrangement.barFrames))));

    CHECK(arrangement.project.clips().size() == 1);
    CHECK(registry.undoDepth() == 0);   // a no-op must not cost an undo entry
}

TEST_CASE("a locked clip refuses to move or resize but can still be unlocked")
{
    Arrangement arrangement;
    app::CommandRegistry registry{arrangement.project};

    const EntityId clip = arrangement.place(0);

    using Flag = app::SetClipFlagCommand::Flag;
    REQUIRE(registry.execute(std::make_unique<app::SetClipFlagCommand>(clip, Flag::locked, true)));

    CHECK_FALSE(registry.execute(std::make_unique<app::MoveClipCommand>(clip, 48000)));
    CHECK_FALSE(registry.execute(std::make_unique<app::ResizeClipCommand>(clip, 1000)));
    CHECK_FALSE(registry.execute(std::make_unique<app::SetClipFlagCommand>(clip, Flag::muted, true)));

    CHECK(arrangement.project.findClip(clip)->start == 0);

    REQUIRE(registry.execute(std::make_unique<app::SetClipFlagCommand>(clip, Flag::locked, false)));
    REQUIRE(registry.execute(std::make_unique<app::MoveClipCommand>(clip, 48000)));
    CHECK(arrangement.project.findClip(clip)->start == 48000);
}

TEST_CASE("a resize drag is one undo, and duplicate lands after the original")
{
    Arrangement arrangement;
    app::CommandRegistry registry{arrangement.project};

    const EntityId clip = arrangement.place(0);

    REQUIRE(registry.execute(std::make_unique<app::ResizeClipCommand>(clip, 30000)));
    for (project::FrameCount length : {28000, 26000, 24000})
        REQUIRE(registry.executeMerging(std::make_unique<app::ResizeClipCommand>(clip, length)));

    CHECK(registry.undoDepth() == 1);
    CHECK(arrangement.project.findClip(clip)->length == 24000);

    auto duplicate = std::make_unique<app::DuplicateClipCommand>(clip);
    app::DuplicateClipCommand* pointer = duplicate.get();
    REQUIRE(registry.execute(std::move(duplicate)));

    const Clip* copy = arrangement.project.findClip(pointer->createdClip());
    REQUIRE(copy != nullptr);
    CHECK(copy->start == 24000);
    CHECK(copy->source == arrangement.pattern);
}

// ── Tracks ────────────────────────────────────────────────────────────────────

TEST_CASE("a muted track is not played, and solo silences the others")
{
    Arrangement arrangement;
    app::CommandRegistry registry{arrangement.project};

    auto second = std::make_unique<app::AddTrackCommand>(project::TrackType::instrument, "Track 2");
    app::AddTrackCommand* pointer = second.get();
    REQUIRE(registry.execute(std::move(second)));

    const EntityId other = pointer->createdTrack();

    arrangement.place(0);
    arrangement.place(static_cast<project::FramePosition>(arrangement.barFrames), other);

    CHECK(project::compileArrangement(arrangement.project, arrangement.channel).size() == 2);

    using Flag = app::SetTrackFlagCommand::Flag;
    REQUIRE(registry.execute(std::make_unique<app::SetTrackFlagCommand>(
        arrangement.track, Flag::muted, true)));

    CHECK(project::compileArrangement(arrangement.project, arrangement.channel).size() == 1);

    REQUIRE(registry.undo());
    REQUIRE(registry.execute(std::make_unique<app::SetTrackFlagCommand>(other, Flag::soloed, true)));

    const auto soloed = project::compileArrangement(arrangement.project, arrangement.channel);
    REQUIRE(soloed.size() == 1);
    CHECK(soloed[0].startTick == ticksPerQuarterNote * 4);   // only the soloed track's clip
}

TEST_CASE("muting a folder mutes everything inside it")
{
    Arrangement arrangement;
    app::CommandRegistry registry{arrangement.project};

    auto folder = std::make_unique<app::AddTrackCommand>(project::TrackType::folder, "Drums");
    app::AddTrackCommand* pointer = folder.get();
    REQUIRE(registry.execute(std::move(folder)));

    const EntityId group = pointer->createdTrack();

    arrangement.place(0);

    REQUIRE(registry.execute(std::make_unique<app::SetTrackParentCommand>(arrangement.track, group)));
    CHECK(project::compileArrangement(arrangement.project, arrangement.channel).size() == 1);

    REQUIRE(registry.execute(std::make_unique<app::SetTrackFlagCommand>(
        group, app::SetTrackFlagCommand::Flag::muted, true)));

    CHECK(project::compileArrangement(arrangement.project, arrangement.channel).empty());

    REQUIRE(registry.undo());
    CHECK(project::compileArrangement(arrangement.project, arrangement.channel).size() == 1);
}

TEST_CASE("a folder cannot contain itself")
{
    Arrangement arrangement;
    app::CommandRegistry registry{arrangement.project};

    auto folder = std::make_unique<app::AddTrackCommand>(project::TrackType::folder, "Drums");
    app::AddTrackCommand* pointer = folder.get();
    REQUIRE(registry.execute(std::move(folder)));

    const EntityId group = pointer->createdTrack();

    REQUIRE(registry.execute(std::make_unique<app::SetTrackParentCommand>(arrangement.track, group)));

    // Would make a cycle: the folder's parent is the track it already contains.
    CHECK_FALSE(registry.execute(std::make_unique<app::SetTrackParentCommand>(group, arrangement.track)));
    CHECK_FALSE(registry.execute(std::make_unique<app::SetTrackParentCommand>(group, group)));

    CHECK(arrangement.project.trackIsAudible(arrangement.track));
}

TEST_CASE("deleting a track takes its clips and undo brings them back")
{
    Arrangement arrangement;
    app::CommandRegistry registry{arrangement.project};

    arrangement.place(0);
    arrangement.place(static_cast<project::FramePosition>(arrangement.barFrames));

    REQUIRE(registry.execute(std::make_unique<app::DeleteTrackCommand>(arrangement.track)));
    CHECK(arrangement.project.tracks().empty());
    CHECK(arrangement.project.clips().empty());

    REQUIRE(registry.undo());
    CHECK(arrangement.project.tracks().size() == 1);
    CHECK(arrangement.project.clips().size() == 2);
}

// ── Clip gain ─────────────────────────────────────────────────────────────────

TEST_CASE("clip gain applies to the placement and survives a save")
{
    Arrangement arrangement;
    app::CommandRegistry registry{arrangement.project};

    const EntityId clip = arrangement.place(0);

    using Property = app::SetClipValueCommand::Property;
    REQUIRE(registry.execute(std::make_unique<app::SetClipValueCommand>(clip, Property::gain, 0.5)));

    const auto quiet = project::compileArrangement(arrangement.project, arrangement.channel);
    REQUIRE(quiet.size() == 1);
    CHECK(quiet[0].velocity == 50);        // the source note asks for 100

    const fs::path package = fs::temp_directory_path() / "incdaw-clipgain.incdaw";
    fs::remove_all(package);

    REQUIRE(project::ProjectFile::save(arrangement.project, package));

    Project reloaded;
    REQUIRE(project::ProjectFile::load(reloaded, package));

    const Clip* stored = reloaded.findClip(clip);
    REQUIRE(stored != nullptr);
    CHECK(stored->gain == doctest::Approx(0.5));

    const auto reloadedNotes = project::compileArrangement(reloaded, arrangement.channel);
    REQUIRE(reloadedNotes.size() == 1);
    CHECK(reloadedNotes[0].velocity == 50);

    fs::remove_all(package);
}

// ── Phase 9 exit criterion ────────────────────────────────────────────────────

TEST_CASE("a full arrangement plays back sample-accurately")
{
    // docs/ROADMAP.md Phase 9: a full arrangement plays back sample-accurately,
    // and clip gain is applied pre-mixer and recallable from the project file.
    constexpr engine::SampleRate rate  = 48000.0;
    constexpr engine::FrameCount block = 64;

    Arrangement arrangement;

    // Three bars: sound, silence, sound. The gap is the part that matters — an
    // arrangement that plays continuously would pass a test that only looked
    // for signal.
    arrangement.place(0);
    arrangement.place(static_cast<project::FramePosition>(arrangement.barFrames) * 2);

    const engine::TempoMap tempoMap{120.0, rate};

    project::GraphCompileOptions options;
    options.mode         = project::PlaybackMode::song;
    options.sampleRate   = rate;
    options.maxBlockSize = block;
    options.masterGain   = 1.0f;

    const auto compiled = project::compileProjectGraph(arrangement.project, tempoMap, options);
    REQUIRE(compiled.graph != nullptr);
    CHECK(compiled.lengthTicks == ticksPerQuarterNote * 12);

    engine::AudioBufferPool output;
    output.allocate(1, 2, block);

    const auto totalFrames = static_cast<engine::FramePosition>(arrangement.barFrames) * 3;

    std::vector<engine::FramePosition> onsets;
    bool sounding = false;

    for (engine::FramePosition position = 0; position < totalFrames; position += block) {
        output.buffer(0).clear();
        compiled.graph->process(output.buffer(0), block, position);

        REQUIRE_FALSE(output.buffer(0).hasNonFiniteSamples());

        const engine::Sample* samples = output.buffer(0).channel(0);

        for (engine::FrameCount frame = 0; frame < block; ++frame) {
            const bool loud = std::abs(samples[frame]) > 1e-4f;

            if (loud && !sounding)
                onsets.push_back(position + frame);

            sounding = loud || sounding;
        }

        // A note stops between placements, so the state has to be re-evaluated
        // per block rather than latched forever.
        sounding = std::abs(samples[block - 1]) > 1e-4f;
    }

    REQUIRE(onsets.size() == 2);

    // The first audible sample lands within a couple of frames of the clip's
    // start: the synth's attack begins at zero, so the exact frame is silent by
    // definition. What must not happen is signal *before* the clip.
    const auto secondStart = static_cast<engine::FramePosition>(arrangement.barFrames) * 2;

    CHECK(onsets[0] >= 0);
    CHECK(onsets[0] < 16);
    CHECK(onsets[1] >= secondStart);
    CHECK(onsets[1] < secondStart + 16);
}
