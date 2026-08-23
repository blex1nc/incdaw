// TRACK B (B6) — playlist lanes.
//
// A track becomes more than one clip deep. The audio never needed changing —
// AudioClipNode already sums overlapping placements and compileArrangement
// already emits every clip's notes, both of which the first test here pins —
// so a lane is a model, a geometry and a gesture: which clip a click lands on,
// where each is drawn, and how a drag moves between them.
//
// A track's lane count is derived from its clips rather than stored, so there
// is no `clip.lane < track.lanes` invariant for an edit, an undo or a load to
// break. That is the property most of these tests are really about.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/PlaylistModel.h"
#include "app/commands/ClipCommands.h"
#include "app/commands/TrackCommands.h"
#include "engine/audio/AudioClipNode.h"
#include "engine/core/AudioBufferPool.h"
#include "project/Model.h"
#include "project/PatternCompiler.h"
#include "project/ProjectFile.h"

#include <atomic>
#include <cmath>
#include <filesystem>
#include <memory>

using namespace incdaw;
using engine::FrameCount;
using engine::FramePosition;
using engine::Sample;
using engine::Tick;
using engine::ticksPerQuarterNote;

namespace fs = std::filesystem;

namespace {

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path() / ("incdaw-lane-" + name + "-"
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

/// One track, one pattern, and whatever clips a test places on it.
struct LaneFixture {
    project::Project   project;
    project::EntityId  channel;
    project::EntityId  pattern;
    project::EntityId  track;
    app::PlaylistModel playlist;

    LaneFixture()
    {
        project.tempoMap().setSampleRate(48000.0);

        channel = project.addChannel("Lead").id;

        project::Pattern& source = project.addPattern("P1");
        source.length            = ticksPerQuarterNote * 4;

        project::MidiEvent note;
        note.type     = project::MidiEventType::note;
        note.duration = 120;
        note.key      = 60;
        note.value    = 100;
        source.contentFor(channel).events.push_back(note);

        pattern = source.id;
        track   = project.addTrack(project::TrackType::instrument, "A").id;

        app::PlaylistModel::Viewport viewport;
        viewport.firstTick    = 0;
        viewport.visibleTicks = ticksPerQuarterNote * 4 * 16;
        viewport.width        = 800.0;
        viewport.height       = 600.0;
        playlist.setViewport(viewport);
    }

    project::EntityId place(Tick start, int lane = 0)
    {
        project::Clip& clip = project.addClip(project::ClipType::pattern, track, pattern);
        clip.startTick      = start;
        clip.lengthTicks    = ticksPerQuarterNote * 4;
        clip.lane           = lane;
        return clip.id;
    }

    [[nodiscard]] project::Clip& at(project::EntityId id) { return *project.findClip(id); }
};

Sample tone(FrameCount frame)
{
    return static_cast<Sample>(0.25 + 0.5 * std::sin(0.05 * static_cast<double>(frame)));
}

} // namespace

// ── What lanes are built on ──────────────────────────────────────────────────

TEST_CASE("two clips that overlap in time are both heard, and always were")
{
    // Audio: the node sums placements rather than letting one mask the other.
    engine::AudioClipNode node;

    auto data = std::make_shared<engine::AudioFileData>();
    data->sampleRate   = 48000.0;
    data->channelCount = 1;
    data->frameCount   = 200;
    data->channels.assign(1, std::vector<Sample>(200));
    for (FrameCount frame = 0; frame < 200; ++frame)
        data->channels[0][static_cast<std::size_t>(frame)] = tone(frame);

    for (int lane = 0; lane < 2; ++lane) {
        engine::AudioClipNode::PlacedClip placed;
        placed.audio  = data;
        placed.start  = 0;
        placed.length = 200;
        node.addClip(std::move(placed));
    }

    engine::AudioBufferPool pool;
    pool.allocate(1, 2, 256);
    pool.buffer(0).clear();

    engine::ProcessContext context;
    context.output       = pool.buffer(0);
    context.frameCount   = 256;
    context.sampleRate   = 48000.0;
    context.playPosition = 0;
    context.playing      = true;

    node.process(context);

    CHECK(pool.buffer(0).channel(0)[10] == doctest::Approx(tone(10) * 2.0f));

    // Patterns: both placements compile.
    LaneFixture fixture;
    fixture.place(0, 0);
    fixture.place(0, 1);

    CHECK(project::compileArrangement(fixture.project, fixture.channel).size() == 2);
}

// ── The model ────────────────────────────────────────────────────────────────

TEST_CASE("a track's lane count comes from its clips, not from a stored number")
{
    LaneFixture fixture;

    CHECK(project::trackLaneCount(fixture.project, fixture.track) == 1);

    const project::EntityId deep = fixture.place(0, 2);
    CHECK(project::trackLaneCount(fixture.project, fixture.track) == 3);

    // Removing the deepest clip takes the lanes with it — nothing to keep in
    // step, and nothing left claiming a lane that has no clips.
    CHECK(fixture.project.removeClip(deep));
    CHECK(project::trackLaneCount(fixture.project, fixture.track) == 1);
}

TEST_CASE("a clip placed over another takes the next lane down")
{
    LaneFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, 0)));
    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, 0)));

    REQUIRE(fixture.project.clips().size() == 2);
    CHECK(fixture.project.clips()[0].lane == 0);
    CHECK(fixture.project.clips()[1].lane == 1);

    // A clip that overlaps nothing stays on lane zero: a track is only as
    // deep as it needs to be.
    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, ticksPerQuarterNote * 16)));
    CHECK(fixture.project.clips()[2].lane == 0);
}

TEST_CASE("a duplicate lands beside its source rather than on top of it")
{
    LaneFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId original = fixture.place(0);

    auto command = std::make_unique<app::DuplicateClipsCommand>(
        app::ClipIds{original}, 0, 0);
    app::DuplicateClipsCommand* raw = command.get();
    REQUIRE(registry.execute(std::move(command)));

    REQUIRE(raw->createdClips().size() == 1);
    CHECK(fixture.at(raw->createdClips()[0]).lane == 1);
}

// ── The gesture ──────────────────────────────────────────────────────────────

TEST_CASE("a drag moves clips between lanes, clamped at the top, and undoes")
{
    LaneFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId clip = fixture.place(0);

    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{clip}, 0, 0, 2)));
    CHECK(fixture.at(clip).lane == 2);

    REQUIRE(registry.undo());
    CHECK(fixture.at(clip).lane == 0);

    REQUIRE(registry.redo());
    CHECK(fixture.at(clip).lane == 2);

    // Lane zero is the floor: a drag past it clamps the whole selection, the
    // way a drag past tick zero already does.
    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{clip}, 0, 0, -9)));
    CHECK(fixture.at(clip).lane == 0);
}

TEST_CASE("a lane drag clamps as one so a selection keeps its shape")
{
    LaneFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId top    = fixture.place(0, 0);
    const project::EntityId bottom = fixture.place(ticksPerQuarterNote * 8, 2);

    // The one already at the top pins the pair, so the gap between them holds
    // — and with nothing left to apply, the gesture is not an undo entry.
    CHECK_FALSE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{top, bottom}, 0, 0, -1)));

    CHECK(fixture.at(top).lane == 0);
    CHECK(fixture.at(bottom).lane == 2);

    // Downwards there is no ceiling: a new lane is what the drag creates.
    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{top, bottom}, 0, 0, 1)));
    CHECK(fixture.at(top).lane == 1);
    CHECK(fixture.at(bottom).lane == 3);
}

TEST_CASE("a diagonal drag is one undo entry, lane included")
{
    LaneFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId clip = fixture.place(ticksPerQuarterNote * 4);

    REQUIRE(registry.executeMerging(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{clip}, ticksPerQuarterNote, 0, 1)));
    REQUIRE(registry.executeMerging(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{clip}, ticksPerQuarterNote, 0, 1)));

    CHECK(fixture.at(clip).startTick == ticksPerQuarterNote * 6);
    CHECK(fixture.at(clip).lane == 2);

    REQUIRE(registry.undo());
    CHECK(fixture.at(clip).startTick == ticksPerQuarterNote * 4);
    CHECK(fixture.at(clip).lane == 0);
    CHECK_FALSE(registry.undo());
}

// ── The geometry ─────────────────────────────────────────────────────────────

TEST_CASE("lanes divide the row, and a clip is drawn in its own band")
{
    LaneFixture fixture;

    const project::EntityId first = fixture.place(0, 0);

    const double whole = fixture.playlist.laneHeight(fixture.project, 0);
    CHECK(whole == doctest::Approx(app::PlaylistModel::rowHeight(fixture.project.tracks(), 0)));

    const project::EntityId second = fixture.place(0, 1);

    // Two lanes, so each is half the row — the track's height did not change.
    const double band = fixture.playlist.laneHeight(fixture.project, 0);
    CHECK(band == doctest::Approx(whole / 2.0));

    const auto top    = fixture.playlist.clipRect(fixture.project, fixture.at(first));
    const auto bottom = fixture.playlist.clipRect(fixture.project, fixture.at(second));

    CHECK(top.height == doctest::Approx(band - 2.0));
    CHECK(bottom.y == doctest::Approx(top.y + band));
    CHECK(bottom.y + bottom.height <= fixture.playlist.trackY(fixture.project.tracks(), 1));
}

TEST_CASE("a click lands on the clip in the lane it points at, not the one drawn last")
{
    LaneFixture fixture;

    const project::EntityId first  = fixture.place(0, 0);
    const project::EntityId second = fixture.place(0, 1);

    const auto topRect    = fixture.playlist.clipRect(fixture.project, fixture.at(first));
    const auto bottomRect = fixture.playlist.clipRect(fixture.project, fixture.at(second));

    const std::size_t hitTop =
        fixture.playlist.clipAtPoint(fixture.project, topRect.x + 4.0,
                                     topRect.y + topRect.height / 2.0);
    const std::size_t hitBottom =
        fixture.playlist.clipAtPoint(fixture.project, bottomRect.x + 4.0,
                                     bottomRect.y + bottomRect.height / 2.0);

    REQUIRE(hitTop != app::PlaylistModel::noClip);
    REQUIRE(hitBottom != app::PlaylistModel::noClip);

    // Both clips occupy the same span, and the later one is drawn on top —
    // before lanes, both of these points answered `second`.
    CHECK(fixture.project.clips()[hitTop].id == first);
    CHECK(fixture.project.clips()[hitBottom].id == second);
}

TEST_CASE("a collapsed folder still hides a track however many lanes it has")
{
    LaneFixture fixture;
    app::CommandRegistry registry{fixture.project};

    fixture.place(0, 0);
    const project::EntityId deep = fixture.place(0, 1);

    const project::EntityId folder =
        fixture.project.addTrack(project::TrackType::folder, "Group").id;
    fixture.project.findTrack(fixture.track)->parent = folder;

    REQUIRE(registry.execute(std::make_unique<app::SetTrackCollapsedCommand>(folder, true)));

    const std::size_t row = fixture.project.indexOfTrack(fixture.track);
    CHECK(fixture.playlist.laneHeight(fixture.project, row) == 0.0);
    CHECK(fixture.playlist.laneAtY(fixture.project, row, 10.0) == 0);
    CHECK(fixture.playlist.clipRect(fixture.project, fixture.at(deep)).width == 0.0);
}

// ── Through the project file ─────────────────────────────────────────────────

TEST_CASE("lanes round-trip through the project file")
{
    ScratchDirectory scratch{"roundtrip"};

    LaneFixture fixture;
    const project::EntityId deep = fixture.place(0, 3);

    const fs::path file = scratch.path / "Lanes.incdaw";
    REQUIRE(bool(project::ProjectFile::save(fixture.project, file)));

    project::Project reloaded;
    REQUIRE(bool(project::ProjectFile::load(reloaded, file)));

    REQUIRE(reloaded.findClip(deep) != nullptr);
    CHECK(reloaded.findClip(deep)->lane == 3);
    CHECK(project::trackLaneCount(reloaded, fixture.track) == 4);
}

TEST_CASE("the v1.9 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.9" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    project::Project project;
    const auto result = project::ProjectFile::load(project, fixture);
    REQUIRE(result.succeeded);

    CHECK(project.metadata().title == "Format v1.9 fixture");

    REQUIRE(project.clips().size() == 2);
    CHECK(project.clips()[0].lane == 0);
    CHECK(project.clips()[1].lane == 1);
    CHECK(project.clips()[0].startTick == project.clips()[1].startTick);
    CHECK(project::trackLaneCount(project, project.clips()[0].track) == 2);
}
