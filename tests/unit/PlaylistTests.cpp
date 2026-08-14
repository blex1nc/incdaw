// Phase 9a — the playlist: pattern clips on tracks.
//
// The load-bearing test is the roadmap's exit criterion taken end to end: the
// same pattern placed twice must play the identical phrase at both positions
// *through the compiled graph*, not merely out of the compiler, and editing the
// pattern must change both. Everything else here exists to keep the edits
// reversible and the geometry honest.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/PlaylistModel.h"
#include "app/commands/ClipCommands.h"
#include "app/commands/TrackCommands.h"
#include "engine/transport/TempoMap.h"
#include "project/PatternCompiler.h"
#include "project/ProjectGraphCompiler.h"

#include <chrono>
#include <memory>
#include <vector>

using namespace incdaw;
using incdaw::engine::SequencedNote;
using incdaw::engine::Tick;
using incdaw::engine::ticksPerQuarterNote;

namespace {

constexpr Tick bar = ticksPerQuarterNote * 4;

project::MidiEvent note(Tick tick, int key = 60, Tick duration = 120)
{
    project::MidiEvent event;
    event.type     = project::MidiEventType::note;
    event.tick     = tick;
    event.duration = duration;
    event.key      = key;
    event.value    = 100;
    return event;
}

/// Note starts relative to an origin — "the same phrase, somewhere else".
std::vector<Tick> shapeFrom(const std::vector<SequencedNote>& notes, Tick origin, Tick until)
{
    std::vector<Tick> shape;
    for (const SequencedNote& entry : notes)
        if (entry.startTick >= origin && entry.startTick < until)
            shape.push_back(entry.startTick - origin);

    return shape;
}

/// A project with one channel, one pattern of three notes, and two tracks.
struct Fixture {
    project::Project    project;
    project::EntityId   channel;
    project::EntityId   pattern;
    project::EntityId   trackA;
    project::EntityId   trackB;

    Fixture()
    {
        channel = project.addChannel("Channel 1").id;
        pattern = project.addPattern("Pattern 1").id;

        auto& events = project.findPattern(pattern)->contentFor(channel).events;
        events.push_back(note(0));
        events.push_back(note(ticksPerQuarterNote, 64));
        events.push_back(note(ticksPerQuarterNote * 2, 67));

        trackA = project.addTrack(project::TrackType::instrument, "Track 1").id;
        trackB = project.addTrack(project::TrackType::instrument, "Track 2").id;
    }
};

} // namespace

// ── The exit criterion ────────────────────────────────────────────────────────

TEST_CASE("a pattern placed twice plays identically at both placements, through the graph")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(
        std::make_unique<app::AddPatternClipCommand>(fixture.trackA, fixture.pattern, 0)));
    REQUIRE(registry.execute(
        std::make_unique<app::AddPatternClipCommand>(fixture.trackB, fixture.pattern, bar * 4)));

    engine::TempoMap tempo;
    tempo.setSampleRate(48000.0);

    project::GraphCompileOptions options;
    options.source = project::PlaybackSource::arrangement;

    const auto compiled = project::compileProjectGraph(fixture.project, tempo, options);
    REQUIRE(compiled);

    engine::InstrumentNode* instrument = compiled.instrumentFor(fixture.channel);
    REQUIRE(instrument != nullptr);

    const std::vector<SequencedNote>& notes = instrument->sequence().notes();

    const std::vector<Tick> first  = shapeFrom(notes, 0, bar);
    const std::vector<Tick> second = shapeFrom(notes, bar * 4, bar * 5);

    REQUIRE(first.size() == 3);
    CHECK(first == second);

    // Editing the pattern changes every placement: the clips reference it, they
    // do not own a copy of it.
    fixture.project.findPattern(fixture.pattern)
        ->contentFor(fixture.channel)
        .events.push_back(note(ticksPerQuarterNote * 3, 72));

    const auto recompiled = project::compileProjectGraph(fixture.project, tempo, options);
    REQUIRE(recompiled);

    const std::vector<SequencedNote>& after = recompiled.instrumentFor(fixture.channel)->sequence().notes();
    CHECK(shapeFrom(after, 0, bar).size() == 4);
    CHECK(shapeFrom(after, bar * 4, bar * 5).size() == 4);
    CHECK(shapeFrom(after, 0, bar) == shapeFrom(after, bar * 4, bar * 5));
}

TEST_CASE("song mode plays the arrangement; pattern mode plays one pattern on loop")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(
        std::make_unique<app::AddPatternClipCommand>(fixture.trackA, fixture.pattern, bar * 2)));

    engine::TempoMap tempo;
    tempo.setSampleRate(48000.0);

    project::GraphCompileOptions options;
    options.source  = project::PlaybackSource::pattern;
    options.pattern = fixture.pattern;

    const auto patternMode = project::compileProjectGraph(fixture.project, tempo, options);
    REQUIRE(patternMode);

    const auto& patternNotes = patternMode.instrumentFor(fixture.channel)->sequence().notes();
    REQUIRE(patternNotes.size() == 3);
    CHECK(patternNotes.front().startTick == 0);

    options.source = project::PlaybackSource::arrangement;
    const auto songMode = project::compileProjectGraph(fixture.project, tempo, options);
    REQUIRE(songMode);

    const auto& songNotes = songMode.instrumentFor(fixture.channel)->sequence().notes();
    REQUIRE(songNotes.size() == 3);
    CHECK(songNotes.front().startTick == bar * 2);

    CHECK(project::arrangementLengthTicks(fixture.project) == bar * 3);
}

// ── Track commands ────────────────────────────────────────────────────────────

TEST_CASE("track commands round trip")
{
    project::Project project;
    app::CommandRegistry registry{project};

    const project::Project original = project;

    auto add = std::make_unique<app::AddTrackCommand>("Track 1");
    app::AddTrackCommand* raw = add.get();
    REQUIRE(registry.execute(std::move(add)));

    const project::EntityId track = raw->trackId();
    CHECK(track.isValid());

    REQUIRE(registry.execute(std::make_unique<app::RenameTrackCommand>(track, "Drums")));
    REQUIRE(registry.execute(std::make_unique<app::SetTrackMutedCommand>(track, true)));
    REQUIRE(registry.execute(std::make_unique<app::SetTrackSoloedCommand>(track, true)));
    REQUIRE(registry.execute(std::make_unique<app::SetTrackHeightCommand>(track, 120)));

    CHECK(project.findTrack(track)->name == "Drums");
    CHECK(project.findTrack(track)->height == 120);

    while (registry.canUndo())
        REQUIRE(registry.undo());

    CHECK(project == original);

    REQUIRE(registry.redo());
    CHECK(project.tracks().size() == 1);
    CHECK(project.tracks().front().id == track);
}

TEST_CASE("removing a track takes its clips with it, and gives them back")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(
        std::make_unique<app::AddPatternClipCommand>(fixture.trackA, fixture.pattern, 0)));
    REQUIRE(registry.execute(
        std::make_unique<app::AddPatternClipCommand>(fixture.trackB, fixture.pattern, bar)));
    REQUIRE(registry.execute(
        std::make_unique<app::AddPatternClipCommand>(fixture.trackA, fixture.pattern, bar * 2)));

    const project::Project before = fixture.project;

    REQUIRE(registry.execute(std::make_unique<app::RemoveTrackCommand>(fixture.trackA)));

    CHECK(fixture.project.tracks().size() == 1);
    CHECK(fixture.project.clips().size() == 1);
    CHECK(fixture.project.clips().front().track == fixture.trackB);

    REQUIRE(registry.undo());
    CHECK(fixture.project == before);
}

TEST_CASE("track mute and solo decide what the arrangement plays")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(
        std::make_unique<app::AddPatternClipCommand>(fixture.trackA, fixture.pattern, 0)));
    REQUIRE(registry.execute(
        std::make_unique<app::AddPatternClipCommand>(fixture.trackB, fixture.pattern, bar * 4)));

    CHECK(project::compileArrangement(fixture.project, fixture.channel).size() == 6);

    REQUIRE(registry.execute(std::make_unique<app::SetTrackMutedCommand>(fixture.trackA, true)));

    const auto muted = project::compileArrangement(fixture.project, fixture.channel);
    REQUIRE(muted.size() == 3);
    CHECK(muted.front().startTick == bar * 4);

    // A muted track does not shorten the song: the arrangement is as long as it
    // was drawn.
    CHECK(project::arrangementLengthTicks(fixture.project) == bar * 5);

    REQUIRE(registry.undo());
    REQUIRE(registry.execute(std::make_unique<app::SetTrackSoloedCommand>(fixture.trackA, true)));

    const auto soloed = project::compileArrangement(fixture.project, fixture.channel);
    REQUIRE(soloed.size() == 3);
    CHECK(soloed.front().startTick == 0);
}

// ── Clip commands ─────────────────────────────────────────────────────────────

TEST_CASE("clip add, move, resize, duplicate, mute and remove all round trip")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::Project original = fixture.project;

    auto add = std::make_unique<app::AddPatternClipCommand>(fixture.trackA, fixture.pattern, bar);
    app::AddPatternClipCommand* rawAdd = add.get();
    REQUIRE(registry.execute(std::move(add)));

    const project::EntityId clip = rawAdd->clipId();
    const app::ClipIds one{clip};

    // A clip with no explicit length takes the pattern's.
    CHECK(fixture.project.findClip(clip)->lengthTicks == fixture.project.findPattern(fixture.pattern)->length);

    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(one, bar, 1)));
    CHECK(fixture.project.findClip(clip)->startTick == bar * 2);
    CHECK(fixture.project.findClip(clip)->track == fixture.trackB);

    REQUIRE(registry.execute(std::make_unique<app::ResizeClipsCommand>(one, bar)));
    CHECK(fixture.project.findClip(clip)->lengthTicks == bar * 2);

    auto duplicate = std::make_unique<app::DuplicateClipsCommand>(one, bar * 4);
    app::DuplicateClipsCommand* rawDuplicate = duplicate.get();
    REQUIRE(registry.execute(std::move(duplicate)));

    REQUIRE(rawDuplicate->createdClips().size() == 1);
    const project::EntityId copy = rawDuplicate->createdClips().front();
    CHECK(copy != clip);
    CHECK(fixture.project.findClip(copy)->startTick == bar * 6);
    CHECK(fixture.project.findClip(copy)->lengthTicks == bar * 2);

    REQUIRE(registry.execute(std::make_unique<app::SetClipMutedCommand>(app::ClipIds{copy}, true)));
    CHECK(fixture.project.findClip(copy)->muted);

    REQUIRE(registry.execute(std::make_unique<app::RemoveClipsCommand>(app::ClipIds{clip, copy})));
    CHECK(fixture.project.clips().empty());

    while (registry.canUndo())
        REQUIRE(registry.undo());

    CHECK(fixture.project == original);
}

TEST_CASE("dragging clips is one undo, and stops at the start of the timeline")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    auto add = std::make_unique<app::AddPatternClipCommand>(fixture.trackA, fixture.pattern, bar);
    app::AddPatternClipCommand* raw = add.get();
    REQUIRE(registry.execute(std::move(add)));

    const app::ClipIds one{raw->clipId()};

    for (int step = 0; step < 4; ++step)
        REQUIRE(registry.executeMerging(std::make_unique<app::MoveClipsCommand>(one, ticksPerQuarterNote, 0)));

    CHECK(registry.undoDepth() == 2);   // the add, then the whole drag
    CHECK(fixture.project.findClip(raw->clipId())->startTick == bar + ticksPerQuarterNote * 4);

    REQUIRE(registry.undo());
    CHECK(fixture.project.findClip(raw->clipId())->startTick == bar);

    // Dragging further left than the timeline start clamps rather than going
    // negative, and undo reverses what happened rather than what was asked.
    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(one, -bar * 4, 0)));
    CHECK(fixture.project.findClip(raw->clipId())->startTick == 0);

    REQUIRE(registry.undo());
    CHECK(fixture.project.findClip(raw->clipId())->startTick == bar);
}

TEST_CASE("a group drag keeps the clips' relative positions")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    auto first = std::make_unique<app::AddPatternClipCommand>(fixture.trackA, fixture.pattern, 0);
    app::AddPatternClipCommand* rawFirst = first.get();
    REQUIRE(registry.execute(std::move(first)));

    auto second = std::make_unique<app::AddPatternClipCommand>(fixture.trackA, fixture.pattern, bar * 2);
    app::AddPatternClipCommand* rawSecond = second.get();
    REQUIRE(registry.execute(std::move(second)));

    const app::ClipIds both{rawFirst->clipId(), rawSecond->clipId()};

    // The leading clip is already at zero, so the whole group must stay put.
    REQUIRE_FALSE(registry.execute(std::make_unique<app::MoveClipsCommand>(both, -bar, 0)));
    CHECK(fixture.project.findClip(rawFirst->clipId())->startTick == 0);
    CHECK(fixture.project.findClip(rawSecond->clipId())->startTick == bar * 2);
}

TEST_CASE("resizing a clip trims or repeats its pattern")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    auto add = std::make_unique<app::AddPatternClipCommand>(fixture.trackA, fixture.pattern, 0);
    app::AddPatternClipCommand* raw = add.get();
    REQUIRE(registry.execute(std::move(add)));

    const app::ClipIds one{raw->clipId()};

    // Twice the pattern length repeats it: three notes become six.
    REQUIRE(registry.execute(std::make_unique<app::ResizeClipsCommand>(one, bar)));
    CHECK(project::compileArrangement(fixture.project, fixture.channel).size() == 6);

    // Half the pattern length trims it to the notes that fit.
    REQUIRE(registry.execute(std::make_unique<app::ResizeClipsCommand>(one, -bar - bar / 2)));
    CHECK(project::compileArrangement(fixture.project, fixture.channel).size() == 2);
}

// ── Playlist geometry ─────────────────────────────────────────────────────────

TEST_CASE("playlist hit testing finds clips, resize handles and empty space")
{
    Fixture fixture;

    project::Clip& clip = fixture.project.addClip(project::ClipType::pattern, fixture.trackA,
                                                  fixture.pattern);
    clip.startTick   = bar;
    clip.lengthTicks = bar * 2;

    app::PlaylistModel playlist;
    app::PlaylistModel::Viewport viewport;
    viewport.firstTick    = 0;
    viewport.visibleTicks = bar * 16;
    viewport.width        = 1600.0;   // 100 points per bar
    viewport.height       = 400.0;
    playlist.setViewport(viewport);

    const auto rect = playlist.clipRect(fixture.project, clip);
    CHECK(rect.x == doctest::Approx(100.0));
    CHECK(rect.width == doctest::Approx(200.0));

    const std::size_t hit = playlist.clipAtPoint(fixture.project, 150.0, rect.y + 4.0);
    REQUIRE(hit != app::PlaylistModel::noClip);
    CHECK(fixture.project.clips()[hit].id == clip.id);

    CHECK_FALSE(playlist.isOverResizeHandle(fixture.project, hit, 150.0, rect.y + 4.0));
    CHECK(playlist.isOverResizeHandle(fixture.project, hit, rect.x + rect.width - 2.0, rect.y + 4.0));

    // Empty timeline, and a point on the second track.
    CHECK(playlist.clipAtPoint(fixture.project, 900.0, rect.y + 4.0) == app::PlaylistModel::noClip);
    CHECK(playlist.trackAtY(fixture.project.tracks(), rect.y + 4.0) == 0);
    CHECK(playlist.trackAtY(fixture.project.tracks(), 70.0) == 1);
    CHECK(playlist.trackAtY(fixture.project.tracks(), 5000.0) == app::PlaylistModel::noTrack);

    std::vector<project::EntityId> boxed;
    playlist.clipsInRectangle(fixture.project, 50.0, 0.0, 400.0, 200.0, boxed);
    REQUIRE(boxed.size() == 1);
    CHECK(boxed.front() == clip.id);

    playlist.clipsInRectangle(fixture.project, 900.0, 0.0, 100.0, 200.0, boxed);
    CHECK(boxed.empty());
}

TEST_CASE("playlist culling drops clips outside the viewport")
{
    Fixture fixture;

    for (int index = 0; index < 200; ++index) {
        project::Clip& clip = fixture.project.addClip(project::ClipType::pattern, fixture.trackA,
                                                      fixture.pattern);
        clip.startTick   = static_cast<Tick>(index) * bar;
        clip.lengthTicks = bar;
    }

    app::PlaylistModel playlist;
    app::PlaylistModel::Viewport viewport;
    viewport.firstTick    = bar * 100;
    viewport.visibleTicks = bar * 8;
    viewport.width        = 800.0;
    viewport.height       = 400.0;
    playlist.setViewport(viewport);

    std::vector<app::PlaylistModel::VisibleClip> visible;
    playlist.collectVisibleClips(fixture.project, visible);

    // Eight bars on screen, plus the clip straddling each edge.
    CHECK(visible.size() <= 10);
    CHECK(visible.size() >= 8);

    for (const auto& entry : visible)
        CHECK(entry.rect.x + entry.rect.width > 0.0);
}

TEST_CASE("playlist snap rounds to the nearest grid line")
{
    app::PlaylistModel playlist;
    playlist.setSnap(bar);

    CHECK(playlist.snapTick(0) == 0);
    CHECK(playlist.snapTick(bar / 2 - 1) == 0);
    CHECK(playlist.snapTick(bar / 2) == bar);
    CHECK(playlist.snapTick(bar * 3 + 10) == bar * 3);

    playlist.setSnap(0);
    CHECK(playlist.snapTick(bar * 3 + 10) == bar * 3 + 10);
}

TEST_CASE("playlist selection survives edits and drops deleted clips")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    auto add = std::make_unique<app::AddPatternClipCommand>(fixture.trackA, fixture.pattern, 0);
    app::AddPatternClipCommand* raw = add.get();
    REQUIRE(registry.execute(std::move(add)));

    app::PlaylistModel playlist;
    playlist.setSelection({raw->clipId()});
    CHECK(playlist.isSelected(raw->clipId()));

    playlist.toggleSelection(raw->clipId());
    CHECK_FALSE(playlist.isSelected(raw->clipId()));

    playlist.addToSelection(raw->clipId());
    REQUIRE(registry.execute(std::make_unique<app::RemoveClipsCommand>(app::ClipIds{raw->clipId()})));

    playlist.pruneSelection(fixture.project);
    CHECK(playlist.selection().empty());
}

TEST_CASE("recompiling a full arrangement is cheap enough to do on every clip drag")
{
    // Every clip edit recompiles the whole graph and swaps it in. That is only
    // defensible if a recompile costs far less than the gap between two mouse
    // moves, so the cost is measured rather than assumed.
    project::Project project;

    const project::EntityId channel = project.addChannel("Channel 1").id;
    const project::EntityId pattern = project.addPattern("Pattern 1").id;

    auto& events = project.findPattern(pattern)->contentFor(channel).events;
    for (int index = 0; index < 64; ++index)
        events.push_back(note(static_cast<Tick>(index) * (ticksPerQuarterNote / 4), 48 + index % 24));

    for (int track = 0; track < 8; ++track) {
        const project::EntityId trackId =
            project.addTrack(project::TrackType::instrument, "Track").id;

        for (int index = 0; index < 64; ++index) {
            project::Clip& clip = project.addClip(project::ClipType::pattern, trackId, pattern);
            clip.startTick   = static_cast<Tick>(index) * bar;
            clip.lengthTicks = bar;
        }
    }

    const auto started = std::chrono::steady_clock::now();

    constexpr int passes = 10;
    std::size_t total = 0;
    for (int pass = 0; pass < passes; ++pass)
        total += project::compileArrangement(project, channel).size();

    const auto elapsed = std::chrono::steady_clock::now() - started;
    const double perCompile =
        std::chrono::duration<double, std::milli>(elapsed).count() / passes;

    MESSAGE("arrangement recompile, 512 clips x 64 notes: " << perCompile << " ms");

    CHECK(total > 0);

    // Generous, because a Debug build runs this too. The number in the message
    // is the one that matters; this only catches an order-of-magnitude
    // regression.
    CHECK(perCompile < 500.0);
}

