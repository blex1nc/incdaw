// TRACK B (B4) — folder tracks.
//
// A folder is a track like any other in the same list; what makes it a folder
// is that other tracks name it as their `parent`. Three properties are worth
// pinning and one is worth pinning twice:
//
//   * mute and solo propagate down, in BOTH compilers — the arrangement
//     compiler decides pattern clips, the graph compiler decides audio tracks,
//     and a folder that muted one but not the other would be worse than no
//     folder at all;
//   * collapse does not. A closed folder is a tidy view, not a silent group;
//   * a cycle is refused before it exists, and a project that has one anyway
//     (a hand-edited file) must not hang the compiler.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ClipCommands.h"
#include "app/commands/TrackCommands.h"
#include "project/Model.h"
#include "project/PatternCompiler.h"
#include "project/ProjectFile.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>

using namespace incdaw;
using engine::Tick;
using engine::ticksPerQuarterNote;

namespace fs = std::filesystem;

namespace {

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path() / ("incdaw-folder-" + name + "-"
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

/// A folder holding two instrument tracks, each with one placement of the same
/// one-note pattern, plus a third track outside the folder.
struct FolderFixture {
    project::Project  project;
    project::EntityId channel;
    project::EntityId pattern;
    project::EntityId folder;
    project::EntityId inside;
    project::EntityId alsoInside;
    project::EntityId outside;

    FolderFixture()
    {
        project.tempoMap().setSampleRate(48000.0);

        channel = project.addChannel("Lead").id;

        project::Pattern& source = project.addPattern("P1");
        source.length            = ticksPerQuarterNote * 4;

        project::MidiEvent note;
        note.type     = project::MidiEventType::note;
        note.tick     = 0;
        note.duration = 120;
        note.key      = 60;
        note.value    = 100;
        source.contentFor(channel).events.push_back(note);

        pattern = source.id;

        folder     = project.addTrack(project::TrackType::folder, "Group").id;
        inside     = project.addTrack(project::TrackType::instrument, "A").id;
        alsoInside = project.addTrack(project::TrackType::instrument, "B").id;
        outside    = project.addTrack(project::TrackType::instrument, "C").id;

        project.findTrack(inside)->parent     = folder;
        project.findTrack(alsoInside)->parent = folder;

        place(inside, 0);
        place(alsoInside, ticksPerQuarterNote * 4);
        place(outside, ticksPerQuarterNote * 8);
    }

    void place(project::EntityId track, Tick start)
    {
        project::Clip& clip = project.addClip(project::ClipType::pattern, track, pattern);
        clip.startTick      = start;
        clip.lengthTicks    = ticksPerQuarterNote * 4;
    }

    [[nodiscard]] std::size_t audibleNotes() const
    {
        return project::compileArrangement(project, channel).size();
    }

    [[nodiscard]] project::Track& track(project::EntityId id)
    {
        return *project.findTrack(id);
    }
};

} // namespace

// ── The tree ─────────────────────────────────────────────────────────────────

TEST_CASE("a folder's children know they are under it")
{
    FolderFixture fixture;

    const auto under = project::tracksUnder(fixture.project, fixture.folder);
    REQUIRE(under.size() == 2);
    CHECK(under[0] == fixture.inside);
    CHECK(under[1] == fixture.alsoInside);

    CHECK(project::tracksUnder(fixture.project, fixture.outside).empty());
}

TEST_CASE("a parent that does not resolve is a root, not a crash")
{
    FolderFixture fixture;

    fixture.track(fixture.inside).parent = project::EntityId{99999};

    CHECK_FALSE(project::trackEffectivelyMuted(fixture.project, fixture.track(fixture.inside)));
    CHECK_FALSE(project::trackHidden(fixture.project, fixture.track(fixture.inside)));
    CHECK(project::tracksUnder(fixture.project, fixture.folder).size() == 1);
}

TEST_CASE("a cycle that a hand-edited file smuggled in does not hang the walk")
{
    FolderFixture fixture;

    // Reachable only by editing the file: the command refuses to build this.
    fixture.track(fixture.folder).parent = fixture.inside;

    CHECK_FALSE(project::trackEffectivelyMuted(fixture.project, fixture.track(fixture.inside)));

    fixture.track(fixture.outside).muted = true;
    CHECK(project::trackEffectivelyMuted(fixture.project, fixture.track(fixture.outside)));
}

// ── Mute and solo propagate ──────────────────────────────────────────────────

TEST_CASE("muting a folder silences everything under it and nothing else")
{
    FolderFixture fixture;
    app::CommandRegistry registry{fixture.project};

    CHECK(fixture.audibleNotes() == 3);

    REQUIRE(registry.execute(std::make_unique<app::SetTrackMutedCommand>(fixture.folder, true)));
    CHECK(fixture.audibleNotes() == 1);   // only the track outside the folder

    REQUIRE(registry.undo());
    CHECK(fixture.audibleNotes() == 3);
}

TEST_CASE("soloing a folder lets its whole group through")
{
    FolderFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::SetTrackSoloedCommand>(fixture.folder, true)));

    // Both children, without either of them carrying a solo flag of its own.
    CHECK(fixture.audibleNotes() == 2);
    CHECK_FALSE(fixture.track(fixture.inside).soloed);
    CHECK_FALSE(fixture.track(fixture.alsoInside).soloed);

    REQUIRE(registry.undo());
    CHECK(fixture.audibleNotes() == 3);
}

TEST_CASE("a muted folder wins over a soloed child, as a muted track always has")
{
    FolderFixture fixture;

    fixture.track(fixture.folder).muted  = true;
    fixture.track(fixture.inside).soloed = true;

    CHECK(fixture.audibleNotes() == 0);   // solo is exclusive; the group is muted
}

TEST_CASE("collapsing a folder changes what is drawn, not what is heard")
{
    FolderFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(
        std::make_unique<app::SetTrackCollapsedCommand>(fixture.folder, true)));

    CHECK(fixture.audibleNotes() == 3);
    CHECK(project::trackHidden(fixture.project, fixture.track(fixture.inside)));
    CHECK_FALSE(project::trackHidden(fixture.project, fixture.track(fixture.outside)));

    // The folder itself is never hidden by its own collapse.
    CHECK_FALSE(project::trackHidden(fixture.project, fixture.track(fixture.folder)));

    REQUIRE(registry.undo());
    CHECK_FALSE(project::trackHidden(fixture.project, fixture.track(fixture.inside)));
}

TEST_CASE("only a folder collapses")
{
    FolderFixture fixture;
    app::CommandRegistry registry{fixture.project};

    CHECK_FALSE(registry.execute(
        std::make_unique<app::SetTrackCollapsedCommand>(fixture.outside, true)));
}

// ── Reparenting ──────────────────────────────────────────────────────────────

TEST_CASE("a track moved into a folder lands next to it in the list")
{
    FolderFixture fixture;
    app::CommandRegistry registry{fixture.project};

    // Order starts as folder, inside, alsoInside, outside.
    REQUIRE(registry.execute(
        std::make_unique<app::SetTrackParentCommand>(fixture.outside, fixture.folder)));

    const auto& tracks = fixture.project.tracks();
    REQUIRE(tracks.size() == 4);
    CHECK(tracks[0].id == fixture.folder);
    CHECK(tracks[3].id == fixture.outside);   // after the folder's existing pair
    CHECK(tracks[3].parent == fixture.folder);

    REQUIRE(registry.undo());
    CHECK(fixture.project.tracks()[3].id == fixture.outside);
    CHECK_FALSE(fixture.project.tracks()[3].parent.isValid());
}

TEST_CASE("a track moved out of a folder goes to the end of the list")
{
    FolderFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(
        std::make_unique<app::SetTrackParentCommand>(fixture.inside, project::EntityId{})));

    const auto& tracks = fixture.project.tracks();
    CHECK(tracks.back().id == fixture.inside);
    CHECK_FALSE(tracks.back().parent.isValid());
    CHECK(fixture.audibleNotes() == 3);
}

TEST_CASE("a folder moved into another folder takes its children with it")
{
    FolderFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId outer =
        fixture.project.addTrack(project::TrackType::folder, "Outer").id;

    REQUIRE(registry.execute(
        std::make_unique<app::SetTrackParentCommand>(fixture.folder, outer)));

    const auto& tracks = fixture.project.tracks();
    const auto index   = [&tracks](project::EntityId id) {
        return static_cast<std::size_t>(std::distance(
            tracks.begin(),
            std::find_if(tracks.begin(), tracks.end(),
                         [id](const project::Track& track) { return track.id == id; })));
    };

    CHECK(index(fixture.folder) == index(outer) + 1);
    CHECK(index(fixture.inside) == index(outer) + 2);
    CHECK(index(fixture.alsoInside) == index(outer) + 3);

    // Muting the outer folder now reaches two levels down.
    fixture.track(outer).muted = true;
    CHECK(fixture.audibleNotes() == 1);
}

TEST_CASE("a reparent that would make a loop, or use a track as a folder, is refused")
{
    FolderFixture fixture;
    app::CommandRegistry registry{fixture.project};

    // Into itself.
    CHECK_FALSE(registry.execute(
        std::make_unique<app::SetTrackParentCommand>(fixture.folder, fixture.folder)));

    // Into its own child.
    CHECK_FALSE(registry.execute(
        std::make_unique<app::SetTrackParentCommand>(fixture.folder, fixture.inside)));

    // Into a track that is not a folder.
    CHECK_FALSE(registry.execute(
        std::make_unique<app::SetTrackParentCommand>(fixture.inside, fixture.outside)));

    CHECK(fixture.track(fixture.folder).parent.isValid() == false);
    CHECK(fixture.track(fixture.inside).parent == fixture.folder);
}

// ── Removing a folder ────────────────────────────────────────────────────────

TEST_CASE("removing a folder keeps the tracks that were in it")
{
    FolderFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::RemoveTrackCommand>(fixture.folder)));

    REQUIRE(fixture.project.tracks().size() == 3);
    CHECK(fixture.project.findTrack(fixture.inside) != nullptr);
    CHECK_FALSE(fixture.track(fixture.inside).parent.isValid());
    CHECK(fixture.audibleNotes() == 3);   // and they are heard again

    REQUIRE(registry.undo());
    CHECK(fixture.project.tracks().size() == 4);
    CHECK(fixture.track(fixture.inside).parent == fixture.folder);
}

TEST_CASE("removing a nested folder hands its children to the folder above")
{
    FolderFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId outer =
        fixture.project.addTrack(project::TrackType::folder, "Outer").id;
    fixture.track(fixture.folder).parent = outer;

    REQUIRE(registry.execute(std::make_unique<app::RemoveTrackCommand>(fixture.folder)));

    CHECK(fixture.track(fixture.inside).parent == outer);
    CHECK(fixture.track(fixture.alsoInside).parent == outer);

    REQUIRE(registry.undo());
    CHECK(fixture.track(fixture.inside).parent == fixture.folder);
}

// ── Colour ───────────────────────────────────────────────────────────────────

TEST_CASE("a track's colour is one undoable command")
{
    FolderFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const std::uint32_t before = fixture.track(fixture.folder).colour;

    REQUIRE(registry.execute(
        std::make_unique<app::SetTrackColourCommand>(fixture.folder, 0xFF224466u)));
    CHECK(fixture.track(fixture.folder).colour == 0xFF224466u);

    CHECK_FALSE(registry.execute(
        std::make_unique<app::SetTrackColourCommand>(fixture.folder, 0xFF224466u)));

    REQUIRE(registry.undo());
    CHECK(fixture.track(fixture.folder).colour == before);
}

// ── The format ───────────────────────────────────────────────────────────────

TEST_CASE("folders, parents and collapsed state round-trip through the project file")
{
    ScratchDirectory scratch{"roundtrip"};

    FolderFixture fixture;
    fixture.track(fixture.folder).collapsed = true;
    fixture.track(fixture.folder).colour    = 0xFF112233u;

    const fs::path file = scratch.path / "Folders.incdaw";
    REQUIRE(bool(project::ProjectFile::save(fixture.project, file)));
    CHECK(project::ProjectFile::versionOf(file) == "1.7");

    project::Project reloaded;
    REQUIRE(bool(project::ProjectFile::load(reloaded, file)));

    const project::Track* folder = reloaded.findTrack(fixture.folder);
    REQUIRE(folder != nullptr);
    CHECK(folder->type == project::TrackType::folder);
    CHECK(folder->collapsed);
    CHECK(folder->colour == 0xFF112233u);

    const project::Track* child = reloaded.findTrack(fixture.inside);
    REQUIRE(child != nullptr);
    CHECK(child->parent == fixture.folder);
    CHECK_FALSE(child->collapsed);
    CHECK(project::trackHidden(reloaded, *child));
}

TEST_CASE("a 1.6 project opens, with its folders open")
{
    ScratchDirectory scratch{"v16"};
    const fs::path package = scratch.path / "Old.incdaw";

    fs::create_directories(package);

    {
        std::ofstream manifest{package / "manifest.json"};
        manifest << R"({"incdaw_project_version": "1.6",
                        "created_with": "INCDAW 0.9.0",
                        "last_saved_with": "INCDAW 0.9.0"})";
    }

    // A 1.6 document: `type` and `parent` were already written, `collapsed`
    // did not exist. Reading it back with every folder open is what those
    // projects looked like, because 1.6 had no way to close one.
    {
        std::ofstream project{package / "project.json"};
        project << R"({
          "metadata": {"title": "Folders before 1.7"},
          "masterMixerNode": 1,
          "nextEntityId": 10,
          "mixerNodes": [{"id": 1, "type": 2, "name": "Master"}],
          "tracks": [
            {"id": 2, "type": 3, "name": "Group", "colour": 4278255360,
             "parent": 0, "outputMixerNode": 1, "muted": true,
             "soloed": false, "height": 40},
            {"id": 3, "type": 0, "name": "A", "colour": 4278190335,
             "parent": 2, "outputMixerNode": 1, "muted": false,
             "soloed": false, "height": 64}
          ]
        })";
    }

    project::Project project;
    const auto result = project::ProjectFile::load(project, package);

    REQUIRE(result.succeeded);
    CHECK(result.migrated);
    CHECK(result.migratedFrom == "1.6");

    REQUIRE(project.tracks().size() == 2);

    const project::Track& folder = project.tracks()[0];
    CHECK(folder.type == project::TrackType::folder);
    CHECK(folder.name == "Group");
    CHECK(folder.muted);
    CHECK(folder.height == 40);
    CHECK_FALSE(folder.collapsed);   // the field the version added

    const project::Track& child = project.tracks()[1];
    CHECK(child.parent == folder.id);
    CHECK_FALSE(project::trackHidden(project, child));

    // The grouping the old file described is live under the new build: the
    // folder's mute now reaches the child, which is the point of the version.
    CHECK(project::trackEffectivelyMuted(project, child));
}

TEST_CASE("the v1.7 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.7" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    project::Project project;
    const auto result = project::ProjectFile::load(project, fixture);
    REQUIRE(result.succeeded);

    CHECK(project.metadata().title == "Format v1.7 fixture");

    REQUIRE(project.tracks().size() == 3);

    const project::Track& folder = project.tracks()[0];
    CHECK(folder.type == project::TrackType::folder);
    CHECK(folder.collapsed);
    CHECK(folder.height == 48);

    // The two tracks under it: one instrument, one audio, both hidden by the
    // folder's collapse and neither collapsed itself.
    CHECK(project.tracks()[1].parent == folder.id);
    CHECK(project.tracks()[2].parent == folder.id);
    CHECK(project::trackHidden(project, project.tracks()[1]));
    CHECK(project::trackHidden(project, project.tracks()[2]));
    CHECK_FALSE(project.tracks()[1].collapsed);
}
