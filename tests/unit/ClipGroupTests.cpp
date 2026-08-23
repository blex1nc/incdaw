// TRACK B (B5) — clip groups.
//
// A group is project state, not a selection: it is saved with the song and it
// is still there when the file is reopened. What that buys is that a verb
// aimed at one member is aimed at all of them — which is enforced inside the
// commands, so a drag, a menu item, a shortcut and an eventual script all get
// it, and which is therefore what these tests look at.
//
// Two interactions are worth pinning beyond the obvious: a copy of a group is
// a group of its own (or moving the copy would drag the source along), and a
// lock on one member pins the whole group (or the rest would slide out from
// under it).

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ClipCommands.h"
#include "project/Model.h"
#include "project/ProjectFile.h"

#include <algorithm>
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
        : path(fs::temp_directory_path() / ("incdaw-clipgroup-" + name + "-"
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

/// Two tracks, three clips of one pattern: two that the tests group, and one
/// left alone to prove the group's verbs stop at its edge.
struct GroupFixture {
    project::Project  project;
    project::EntityId pattern;
    project::EntityId trackA;
    project::EntityId trackB;
    project::EntityId first;
    project::EntityId second;
    project::EntityId loner;

    GroupFixture()
    {
        project.tempoMap().setSampleRate(48000.0);

        const project::EntityId channel = project.addChannel("Lead").id;

        project::Pattern& source = project.addPattern("P1");
        source.length            = ticksPerQuarterNote * 4;
        (void)source.contentFor(channel);

        pattern = source.id;

        trackA = project.addTrack(project::TrackType::instrument, "A").id;
        trackB = project.addTrack(project::TrackType::instrument, "B").id;

        first  = place(trackA, ticksPerQuarterNote * 4);
        second = place(trackB, ticksPerQuarterNote * 8);
        loner  = place(trackA, ticksPerQuarterNote * 16);
    }

    project::EntityId place(project::EntityId track, Tick start)
    {
        project::Clip& clip = project.addClip(project::ClipType::pattern, track, pattern);
        clip.startTick      = start;
        clip.lengthTicks    = ticksPerQuarterNote * 4;
        return clip.id;
    }

    [[nodiscard]] project::Clip& at(project::EntityId id) { return *project.findClip(id); }

    /// Ties `first` and `second` together, returning the group id.
    project::EntityId group(app::CommandRegistry& registry)
    {
        auto command = std::make_unique<app::GroupClipsCommand>(app::ClipIds{first, second});
        app::GroupClipsCommand* raw = command.get();

        REQUIRE(registry.execute(std::move(command)));
        return raw->groupId();
    }
};

} // namespace

// ── Making one ───────────────────────────────────────────────────────────────

TEST_CASE("grouping ties clips together and undoes cleanly")
{
    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId id = fixture.group(registry);

    REQUIRE(id.isValid());
    CHECK(fixture.at(fixture.first).group == id);
    CHECK(fixture.at(fixture.second).group == id);
    CHECK_FALSE(fixture.at(fixture.loner).group.isValid());

    REQUIRE(registry.undo());
    CHECK_FALSE(fixture.at(fixture.first).group.isValid());

    // Redo lands on the same id rather than minting a second one, so anything
    // that recorded the group is still pointing at it.
    REQUIRE(registry.redo());
    CHECK(fixture.at(fixture.first).group == id);
}

TEST_CASE("one clip is not a group")
{
    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    CHECK_FALSE(registry.execute(
        std::make_unique<app::GroupClipsCommand>(app::ClipIds{fixture.first})));
    CHECK_FALSE(fixture.at(fixture.first).group.isValid());
}

TEST_CASE("grouping a clip that is already grouped folds its whole group in")
{
    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    fixture.group(registry);

    // Naming only `second` and `loner` still catches `first`, because `second`
    // brings its group with it.
    REQUIRE(registry.execute(std::make_unique<app::GroupClipsCommand>(
        app::ClipIds{fixture.second, fixture.loner})));

    const project::EntityId merged = fixture.at(fixture.loner).group;
    REQUIRE(merged.isValid());
    CHECK(fixture.at(fixture.first).group == merged);
    CHECK(fixture.at(fixture.second).group == merged);
}

TEST_CASE("ungrouping breaks the whole group, not the selection")
{
    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    fixture.group(registry);

    REQUIRE(registry.execute(
        std::make_unique<app::UngroupClipsCommand>(app::ClipIds{fixture.first})));

    CHECK_FALSE(fixture.at(fixture.first).group.isValid());
    CHECK_FALSE(fixture.at(fixture.second).group.isValid());

    REQUIRE(registry.undo());
    CHECK(fixture.at(fixture.first).group == fixture.at(fixture.second).group);

    CHECK_FALSE(registry.execute(
        std::make_unique<app::UngroupClipsCommand>(app::ClipIds{fixture.loner})));
}

// ── Editing one ──────────────────────────────────────────────────────────────

TEST_CASE("moving one member moves the group, and the loner stays put")
{
    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    fixture.group(registry);

    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{fixture.first}, ticksPerQuarterNote * 4, 0)));

    CHECK(fixture.at(fixture.first).startTick == ticksPerQuarterNote * 8);
    CHECK(fixture.at(fixture.second).startTick == ticksPerQuarterNote * 12);
    CHECK(fixture.at(fixture.loner).startTick == ticksPerQuarterNote * 16);

    REQUIRE(registry.undo());
    CHECK(fixture.at(fixture.second).startTick == ticksPerQuarterNote * 8);
}

TEST_CASE("deleting one member deletes the group")
{
    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    fixture.group(registry);

    REQUIRE(registry.execute(
        std::make_unique<app::RemoveClipsCommand>(app::ClipIds{fixture.second})));

    CHECK(fixture.project.findClip(fixture.first) == nullptr);
    CHECK(fixture.project.findClip(fixture.second) == nullptr);
    CHECK(fixture.project.findClip(fixture.loner) != nullptr);

    REQUIRE(registry.undo());
    CHECK(fixture.project.findClip(fixture.first) != nullptr);
}

TEST_CASE("resizing one member resizes the group")
{
    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    fixture.group(registry);

    REQUIRE(registry.execute(std::make_unique<app::ResizeClipsCommand>(
        app::ClipIds{fixture.first}, ticksPerQuarterNote)));

    CHECK(fixture.at(fixture.first).lengthTicks == ticksPerQuarterNote * 5);
    CHECK(fixture.at(fixture.second).lengthTicks == ticksPerQuarterNote * 5);
    CHECK(fixture.at(fixture.loner).lengthTicks == ticksPerQuarterNote * 4);
}

TEST_CASE("colouring one member colours the group")
{
    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    fixture.group(registry);

    const std::uint32_t before = fixture.at(fixture.loner).colour;

    REQUIRE(registry.execute(std::make_unique<app::SetClipColourCommand>(
        app::ClipIds{fixture.second}, 0xFF445566u)));

    CHECK(fixture.at(fixture.first).colour == 0xFF445566u);
    CHECK(fixture.at(fixture.second).colour == 0xFF445566u);
    CHECK(fixture.at(fixture.loner).colour == before);

    REQUIRE(registry.undo());
    CHECK(fixture.at(fixture.first).colour != 0xFF445566u);
}

TEST_CASE("a copy of a group is a group of its own")
{
    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId original = fixture.group(registry);

    auto command = std::make_unique<app::DuplicateClipsCommand>(
        app::ClipIds{fixture.first}, ticksPerQuarterNote * 32, 0);
    app::DuplicateClipsCommand* raw = command.get();

    REQUIRE(registry.execute(std::move(command)));

    // Both members were copied, even though only one was named.
    const app::ClipIds& copies = raw->createdClips();
    REQUIRE(copies.size() == 2);

    const project::EntityId copiedGroup = fixture.at(copies[0]).group;
    REQUIRE(copiedGroup.isValid());
    CHECK(copiedGroup != original);
    CHECK(fixture.at(copies[1]).group == copiedGroup);

    // And moving the copy leaves the original where it was.
    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{copies[0]}, ticksPerQuarterNote * 4, 0)));

    CHECK(fixture.at(fixture.first).startTick == ticksPerQuarterNote * 4);
}

TEST_CASE("a locked member pins the whole group")
{
    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    fixture.group(registry);

    REQUIRE(registry.execute(std::make_unique<app::SetClipLockedCommand>(
        app::ClipIds{fixture.second}, true)));

    CHECK_FALSE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{fixture.first}, ticksPerQuarterNote * 4, 0)));

    CHECK(fixture.at(fixture.first).startTick == ticksPerQuarterNote * 4);
    CHECK(fixture.at(fixture.second).startTick == ticksPerQuarterNote * 8);

    // The clip outside the group is unaffected by any of it.
    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{fixture.loner}, ticksPerQuarterNote * 4, 0)));
    CHECK(fixture.at(fixture.loner).startTick == ticksPerQuarterNote * 20);
}

TEST_CASE("splitting a grouped clip leaves both halves in the group")
{
    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId id = fixture.group(registry);

    auto command = std::make_unique<app::SplitClipCommand>(fixture.first,
                                                           ticksPerQuarterNote * 6);
    app::SplitClipCommand* raw = command.get();
    REQUIRE(registry.execute(std::move(command)));

    CHECK(fixture.at(fixture.first).group == id);
    CHECK(fixture.at(raw->rightClipId()).group == id);
}

// ── Through the project file ─────────────────────────────────────────────────

TEST_CASE("a group survives a save and load, and still moves as one")
{
    ScratchDirectory scratch{"roundtrip"};

    GroupFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::EntityId id = fixture.group(registry);

    const fs::path file = scratch.path / "Groups.incdaw";
    REQUIRE(bool(project::ProjectFile::save(fixture.project, file)));

    project::Project reloaded;
    REQUIRE(bool(project::ProjectFile::load(reloaded, file)));

    REQUIRE(reloaded.findClip(fixture.first) != nullptr);
    CHECK(reloaded.findClip(fixture.first)->group == id);
    CHECK(reloaded.findClip(fixture.second)->group == id);
    CHECK_FALSE(reloaded.findClip(fixture.loner)->group.isValid());

    // The group is live in the reopened project, not just recorded in it.
    app::CommandRegistry after{reloaded};
    REQUIRE(after.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{fixture.first}, ticksPerQuarterNote * 4, 0)));

    CHECK(reloaded.findClip(fixture.second)->startTick == ticksPerQuarterNote * 12);
}

TEST_CASE("the v1.8 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.8" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    project::Project project;
    const auto result = project::ProjectFile::load(project, fixture);
    REQUIRE(result.succeeded);

    CHECK(project.metadata().title == "Format v1.8 fixture");

    REQUIRE(project.clips().size() == 3);
    CHECK(project.clips()[0].group == project.clips()[1].group);
    CHECK(project.clips()[0].group.isValid());
    CHECK_FALSE(project.clips()[2].group.isValid());
}
