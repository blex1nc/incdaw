// TRACK B (B11) — several timelines in one project.
//
// The design decision worth the tests: clips and markers live INSIDE an
// arrangement, and `Project::clips()` hands back the current one's list. That
// is what lets the compilers, the playlist and every command go on meaning
// exactly what they meant — a flag on each clip would have needed the filter
// applied at every one of those sites, and one missed site is a clip from
// another arrangement playing over this one. So the load-bearing test is that
// switching arrangements changes what compiles, without either compiler having
// been told anything about arrangements.
//
// The second is that switching is a command: the undo stack is one stack
// across timelines, so an edit, a switch and an undo must walk back through
// the switch before reaching the edit.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ArrangementCommands.h"
#include "app/commands/ClipCommands.h"
#include "app/commands/MarkerCommands.h"
#include "project/Model.h"
#include "project/PatternCompiler.h"
#include "project/ProjectFile.h"

#include <atomic>
#include <filesystem>
#include <memory>

using namespace incdaw;
using engine::Tick;
using engine::ticksPerQuarterNote;

namespace fs = std::filesystem;

namespace {

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path() / ("incdaw-arrangement-" + name + "-"
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

/// One channel, one pattern of one note, one track — all shared — and whatever
/// clips a test places on whichever timeline is current.
struct ArrangementFixture {
    project::Project  project;
    project::EntityId channel;
    project::EntityId pattern;
    project::EntityId track;

    ArrangementFixture()
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
    }

    [[nodiscard]] std::size_t audibleNotes() const
    {
        return project::compileArrangement(project, channel).size();
    }
};

} // namespace

// ── The model ────────────────────────────────────────────────────────────────

TEST_CASE("a project has one arrangement from the moment it exists")
{
    project::Project project;

    REQUIRE(project.arrangements().size() == 1);
    CHECK(project.currentArrangement() == project.arrangements()[0].id);
    CHECK(project.arrangements()[0].name == "Arrangement 1");
    CHECK(project.clips().empty());
}

TEST_CASE("clips and markers belong to the arrangement they were made in")
{
    ArrangementFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, 0)));
    REQUIRE(registry.execute(std::make_unique<app::AddMarkerCommand>(
        ticksPerQuarterNote * 8, "Drop")));

    CHECK(fixture.project.clips().size() == 1);
    CHECK(fixture.project.markers().size() == 1);
    CHECK(fixture.audibleNotes() == 1);

    auto command = std::make_unique<app::AddArrangementCommand>("Live");
    app::AddArrangementCommand* raw = command.get();
    REQUIRE(registry.execute(std::move(command)));

    // The new timeline is empty and current; the material is not gone, it is
    // simply not laid out here yet.
    CHECK(fixture.project.currentArrangement() == raw->arrangementId());
    CHECK(fixture.project.clips().empty());
    CHECK(fixture.project.markers().empty());
    CHECK(fixture.audibleNotes() == 0);

    // Everything else is shared.
    CHECK(fixture.project.patterns().size() == 1);
    CHECK(fixture.project.channels().size() == 1);
    CHECK(fixture.project.tracks().size() == 1);

    // Placing here does not touch the first timeline.
    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, ticksPerQuarterNote * 16)));

    CHECK(fixture.audibleNotes() == 1);
    CHECK(fixture.project.clips()[0].startTick == ticksPerQuarterNote * 16);
}

TEST_CASE("switching arrangements changes what compiles, and neither compiler was told")
{
    ArrangementFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId first = fixture.project.currentArrangement();

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, 0)));
    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, ticksPerQuarterNote * 4)));

    CHECK(fixture.audibleNotes() == 2);

    auto command = std::make_unique<app::AddArrangementCommand>("Live");
    app::AddArrangementCommand* raw = command.get();
    REQUIRE(registry.execute(std::move(command)));

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, 0)));

    CHECK(fixture.audibleNotes() == 1);

    REQUIRE(registry.execute(std::make_unique<app::SetCurrentArrangementCommand>(first)));
    CHECK(fixture.audibleNotes() == 2);

    REQUIRE(registry.execute(
        std::make_unique<app::SetCurrentArrangementCommand>(raw->arrangementId())));
    CHECK(fixture.audibleNotes() == 1);
}

// ── Switching is a command ───────────────────────────────────────────────────

TEST_CASE("an edit, a switch and an undo walk back through the switch")
{
    ArrangementFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId first = fixture.project.currentArrangement();

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, 0)));

    auto command = std::make_unique<app::AddArrangementCommand>("Live");
    app::AddArrangementCommand* raw = command.get();
    REQUIRE(registry.execute(std::move(command)));

    CHECK(fixture.project.currentArrangement() == raw->arrangementId());

    // Undoing the new arrangement puts the old one back as current, and the
    // NEXT undo then reaches the clip it made — which it could not do if the
    // switch had bypassed the stack.
    REQUIRE(registry.undo());
    CHECK(fixture.project.currentArrangement() == first);
    CHECK(fixture.project.clips().size() == 1);

    REQUIRE(registry.undo());
    CHECK(fixture.project.clips().empty());

    REQUIRE(registry.redo());
    REQUIRE(registry.redo());
    CHECK(fixture.project.currentArrangement() == raw->arrangementId());
    CHECK(fixture.project.arrangements().size() == 2);
}

TEST_CASE("switching to the arrangement already current is not an undo entry")
{
    project::Project project;
    app::CommandRegistry registry{project};

    CHECK_FALSE(registry.execute(std::make_unique<app::SetCurrentArrangementCommand>(
        project.currentArrangement())));

    CHECK_FALSE(registry.execute(std::make_unique<app::SetCurrentArrangementCommand>(
        project::EntityId{9999})));
}

// ── Duplicating and removing ─────────────────────────────────────────────────

TEST_CASE("a duplicated arrangement is a separate layout of the same material")
{
    ArrangementFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId first = fixture.project.currentArrangement();

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, 0)));
    const project::EntityId original = fixture.project.clips()[0].id;

    auto command = std::make_unique<app::AddArrangementCommand>("Copy", first);
    app::AddArrangementCommand* raw = command.get();
    REQUIRE(registry.execute(std::move(command)));

    REQUIRE(fixture.project.clips().size() == 1);

    // Same layout, fresh identity: two clips sharing an id would make findClip
    // a coin toss the moment both timelines existed.
    const project::Clip& copy = fixture.project.clips()[0];
    CHECK(copy.startTick == 0);
    CHECK(copy.source == fixture.pattern);
    CHECK(copy.id != original);

    // Editing the copy leaves the original alone.
    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{copy.id}, ticksPerQuarterNote * 8, 0)));

    REQUIRE(registry.execute(std::make_unique<app::SetCurrentArrangementCommand>(first)));
    CHECK(fixture.project.clips()[0].startTick == 0);

    // And the pattern really is shared: editing it is heard in both.
    CHECK(fixture.audibleNotes() == 1);
    REQUIRE(registry.execute(
        std::make_unique<app::SetCurrentArrangementCommand>(raw->arrangementId())));
    CHECK(fixture.audibleNotes() == 1);
}

TEST_CASE("removing an arrangement takes its clips with it, and undo brings both back")
{
    ArrangementFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, 0)));

    auto command = std::make_unique<app::AddArrangementCommand>("Live");
    app::AddArrangementCommand* raw = command.get();
    REQUIRE(registry.execute(std::move(command)));

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, ticksPerQuarterNote * 8)));

    REQUIRE(registry.execute(
        std::make_unique<app::RemoveArrangementCommand>(raw->arrangementId())));

    REQUIRE(fixture.project.arrangements().size() == 1);
    CHECK(fixture.project.clips().size() == 1);
    CHECK(fixture.project.clips()[0].startTick == 0);

    REQUIRE(registry.undo());
    REQUIRE(fixture.project.arrangements().size() == 2);
    CHECK(fixture.project.currentArrangement() == raw->arrangementId());
    CHECK(fixture.project.clips()[0].startTick == ticksPerQuarterNote * 8);
}

TEST_CASE("the last arrangement cannot be removed")
{
    project::Project project;
    app::CommandRegistry registry{project};

    CHECK_FALSE(registry.execute(
        std::make_unique<app::RemoveArrangementCommand>(project.currentArrangement())));
    CHECK(project.arrangements().size() == 1);
}

TEST_CASE("arrangements are renamed")
{
    project::Project project;
    app::CommandRegistry registry{project};

    const project::EntityId id = project.currentArrangement();

    REQUIRE(registry.execute(std::make_unique<app::RenameArrangementCommand>(id, "Song")));
    CHECK(project.findArrangement(id)->name == "Song");

    CHECK_FALSE(registry.execute(std::make_unique<app::RenameArrangementCommand>(id, "Song")));

    REQUIRE(registry.undo());
    CHECK(project.findArrangement(id)->name == "Arrangement 1");
}

// ── Through the project file ─────────────────────────────────────────────────

TEST_CASE("arrangements round-trip, and the current one comes back current")
{
    ScratchDirectory scratch{"roundtrip"};

    ArrangementFixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, 0)));
    REQUIRE(registry.execute(std::make_unique<app::AddMarkerCommand>(
        ticksPerQuarterNote * 4, "Drop")));

    auto command = std::make_unique<app::AddArrangementCommand>("Live");
    app::AddArrangementCommand* raw = command.get();
    REQUIRE(registry.execute(std::move(command)));

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(
        fixture.track, fixture.pattern, ticksPerQuarterNote * 16)));

    const fs::path file = scratch.path / "Arrangements.incdaw";
    REQUIRE(bool(project::ProjectFile::save(fixture.project, file)));

    project::Project reloaded;
    REQUIRE(bool(project::ProjectFile::load(reloaded, file)));

    REQUIRE(reloaded.arrangements().size() == 2);
    CHECK(reloaded.arrangements()[0].name == "Arrangement 1");
    CHECK(reloaded.arrangements()[1].name == "Live");

    CHECK(reloaded.currentArrangement() == raw->arrangementId());
    REQUIRE(reloaded.clips().size() == 1);
    CHECK(reloaded.clips()[0].startTick == ticksPerQuarterNote * 16);

    // The first timeline's marker went with it rather than to the current one.
    CHECK(reloaded.markers().empty());
    REQUIRE(reloaded.arrangements()[0].markers.size() == 1);
    CHECK(reloaded.arrangements()[0].markers[0].name == "Drop");

    // And the whole project compares equal, which is what the round-trip test
    // in ProjectFormatTests is really asserting for every other entity.
    CHECK(reloaded == fixture.project);
}

TEST_CASE("a pre-1.11 project opens as one arrangement")
{
    // The 1.10 fixture wrote its clips at the top level, because that is all a
    // 1.10 project had. Opening it must produce exactly one timeline holding
    // them, not an empty one beside them.
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.10" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    project::Project project;
    const auto result = project::ProjectFile::load(project, fixture);
    REQUIRE(result.succeeded);
    CHECK(result.migrated);
    CHECK(result.migratedFrom == "1.10");

    REQUIRE(project.arrangements().size() == 1);
    CHECK(project.arrangements()[0].name == "Arrangement 1");
    CHECK(project.currentArrangement() == project.arrangements()[0].id);
    CHECK(project.clips().size() == 2);

    // The id minted for the arrangement must not be handed out again.
    const project::EntityId minted = project.ids().next();
    CHECK(minted != project.arrangements()[0].id);
    for (const project::Clip& clip : project.clips())
        CHECK(minted != clip.id);
}

TEST_CASE("the v1.11 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.11" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    project::Project project;
    const auto result = project::ProjectFile::load(project, fixture);
    REQUIRE(result.succeeded);

    CHECK(project.metadata().title == "Format v1.11 fixture");

    REQUIRE(project.arrangements().size() == 2);
    CHECK(project.arrangements()[0].name == "Arrangement 1");
    CHECK(project.arrangements()[1].name == "Live");

    // Saved while the second was current, and it comes back current.
    CHECK(project.currentArrangement() == project.arrangements()[1].id);
    REQUIRE(project.clips().size() == 1);
    CHECK(project.clips()[0].startTick == engine::ticksPerQuarterNote * 16);

    // The marker stayed on the timeline it was dropped on.
    CHECK(project.markers().empty());
    REQUIRE(project.arrangements()[0].markers.size() == 1);
    CHECK(project.arrangements()[0].markers[0].name == "Drop");
}
