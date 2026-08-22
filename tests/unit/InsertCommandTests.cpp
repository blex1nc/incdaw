// Phase 13 — insert-chain edits as commands.
//
// Add, remove and bypass are the UI's only way to touch a chain (CLAUDE.md
// §26: every action is a command), so what these tests really pin down is
// undo fidelity: a removed slot comes back in place WITH its stateFile, a
// redone add keeps its slot id, and a no-op bypass refuses to occupy an
// undo step.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/PluginCommands.h"
#include "project/Model.h"

#include <memory>
#include <string>

using namespace incdaw;

namespace {

struct Fixture {
    project::Project    project;
    app::CommandRegistry registry{project};

    project::MixerNode& master()
    {
        for (project::MixerNode& node : project.mixerNodes())
            if (node.id == project.masterMixerNode())
                return node;

        FAIL("no master");
        return project.mixerNodes().front();
    }

    plugins::PluginIdentifier gain()
    {
        plugins::PluginIdentifier plugin;
        plugin.format = plugins::Format::clap;
        plugin.uid    = "com.incdaw.testgain";
        return plugin;
    }
};

} // namespace

TEST_CASE("adding an insert is undoable, and redo keeps the slot id")
{
    Fixture fixture;

    auto command = std::make_unique<app::AddInsertCommand>(fixture.project.masterMixerNode(),
                                                           fixture.gain());
    auto* raw = command.get();

    REQUIRE(fixture.registry.execute(std::move(command)));
    REQUIRE(fixture.master().inserts.size() == 1);

    const project::EntityId slotId = raw->slotId();
    CHECK(slotId.isValid());
    CHECK(fixture.master().inserts.front().id == slotId);

    REQUIRE(fixture.registry.undo());
    CHECK(fixture.master().inserts.empty());

    // The id survives the round trip: an automation lane targeting the slot
    // must still resolve after undo/redo.
    REQUIRE(fixture.registry.redo());
    REQUIRE(fixture.master().inserts.size() == 1);
    CHECK(fixture.master().inserts.front().id == slotId);
}

TEST_CASE("adding to a node that does not exist fails without occupying history")
{
    Fixture fixture;

    CHECK(!fixture.registry.execute(std::make_unique<app::AddInsertCommand>(
        project::EntityId{999999}, fixture.gain())));
    CHECK(!fixture.registry.canUndo());

    // An invalid identifier is refused the same way.
    CHECK(!fixture.registry.execute(std::make_unique<app::AddInsertCommand>(
        fixture.project.masterMixerNode(), plugins::PluginIdentifier{})));
    CHECK(!fixture.registry.canUndo());
}

TEST_CASE("removing an insert brings it back in place with its state")
{
    Fixture fixture;

    // Three slots, so position is provable; the middle one carries state.
    project::EntityId ids[3];
    for (auto& id : ids) {
        auto add = std::make_unique<app::AddInsertCommand>(fixture.project.masterMixerNode(),
                                                           fixture.gain());
        auto* raw = add.get();
        REQUIRE(fixture.registry.execute(std::move(add)));
        id = raw->slotId();
    }

    fixture.master().inserts[1].stateFile = "plugins/insert-42.state";

    REQUIRE(fixture.registry.execute(std::make_unique<app::RemoveInsertCommand>(
        fixture.project.masterMixerNode(), ids[1])));

    REQUIRE(fixture.master().inserts.size() == 2);
    CHECK(fixture.master().inserts[0].id == ids[0]);
    CHECK(fixture.master().inserts[1].id == ids[2]);

    REQUIRE(fixture.registry.undo());

    REQUIRE(fixture.master().inserts.size() == 3);
    CHECK(fixture.master().inserts[1].id == ids[1]);
    CHECK(fixture.master().inserts[1].stateFile == "plugins/insert-42.state");
}

TEST_CASE("moving an insert reorders the chain, undoes, and refuses the ends")
{
    Fixture fixture;

    auto first  = std::make_unique<app::AddInsertCommand>(fixture.project.masterMixerNode(), fixture.gain());
    auto second = std::make_unique<app::AddInsertCommand>(fixture.project.masterMixerNode(), fixture.gain());

    const app::AddInsertCommand* firstAdd  = first.get();
    const app::AddInsertCommand* secondAdd = second.get();

    REQUIRE(fixture.registry.execute(std::move(first)));
    REQUIRE(fixture.registry.execute(std::move(second)));

    const project::EntityId a = firstAdd->slotId();
    const project::EntityId b = secondAdd->slotId();

    auto& inserts = fixture.master().inserts;
    REQUIRE(inserts.size() == 2);
    CHECK(inserts[0].id == a);

    // Up from the top and down from the bottom are refusals, not wraps.
    CHECK_FALSE(fixture.registry.execute(
        std::make_unique<app::MoveInsertCommand>(fixture.project.masterMixerNode(), a, -1)));
    CHECK_FALSE(fixture.registry.execute(
        std::make_unique<app::MoveInsertCommand>(fixture.project.masterMixerNode(), b, 1)));

    REQUIRE(fixture.registry.execute(
        std::make_unique<app::MoveInsertCommand>(fixture.project.masterMixerNode(), b, -1)));
    CHECK(inserts[0].id == b);
    CHECK(inserts[1].id == a);

    fixture.registry.undo();
    CHECK(inserts[0].id == a);
    CHECK(inserts[1].id == b);

    fixture.registry.redo();
    CHECK(inserts[0].id == b);
}

TEST_CASE("bypass toggles, undoes, and refuses a no-op")
{
    Fixture fixture;

    auto add = std::make_unique<app::AddInsertCommand>(fixture.project.masterMixerNode(),
                                                       fixture.gain());
    auto* raw = add.get();
    REQUIRE(fixture.registry.execute(std::move(add)));
    const project::EntityId slotId = raw->slotId();

    REQUIRE(fixture.registry.execute(std::make_unique<app::SetInsertBypassedCommand>(
        fixture.project.masterMixerNode(), slotId, true)));
    CHECK(fixture.master().inserts.front().bypassed);

    // Already bypassed: refused, and the undo stack does not grow.
    const std::size_t depth = fixture.registry.undoDepth();
    CHECK(!fixture.registry.execute(std::make_unique<app::SetInsertBypassedCommand>(
        fixture.project.masterMixerNode(), slotId, true)));
    CHECK(fixture.registry.undoDepth() == depth);

    REQUIRE(fixture.registry.undo());
    CHECK(!fixture.master().inserts.front().bypassed);
}

// ── Dropping a plugin ONTO a slot (UI build-out increment 15) ─────────────────
//
// The mixer's dock lets a plugin be dragged onto a particular link of the
// chain. Chain order is signal order, so "third" has to mean third — on the
// first execute and on every redo after it.

TEST_CASE("an insert can be added at a position, and redo puts it back there")
{
    Fixture fixture;

    plugins::PluginIdentifier first  = fixture.gain();
    plugins::PluginIdentifier second = fixture.gain();
    second.uid = "com.incdaw.second";

    plugins::PluginIdentifier dropped = fixture.gain();
    dropped.uid = "com.incdaw.dropped";

    REQUIRE(fixture.registry.execute(
        std::make_unique<app::AddInsertCommand>(fixture.project.masterMixerNode(), first)));
    REQUIRE(fixture.registry.execute(
        std::make_unique<app::AddInsertCommand>(fixture.project.masterMixerNode(), second)));

    // Between the two, which is what a drop on the second row means.
    REQUIRE(fixture.registry.execute(std::make_unique<app::AddInsertCommand>(
        fixture.project.masterMixerNode(), dropped, 1)));

    REQUIRE(fixture.master().inserts.size() == 3);
    CHECK(fixture.master().inserts[0].plugin.uid == "com.incdaw.testgain");
    CHECK(fixture.master().inserts[1].plugin.uid == "com.incdaw.dropped");
    CHECK(fixture.master().inserts[2].plugin.uid == "com.incdaw.second");

    const project::EntityId slotId = fixture.master().inserts[1].id;

    fixture.registry.undo();
    REQUIRE(fixture.master().inserts.size() == 2);
    CHECK(fixture.master().inserts[1].plugin.uid == "com.incdaw.second");

    fixture.registry.redo();
    REQUIRE(fixture.master().inserts.size() == 3);
    CHECK(fixture.master().inserts[1].plugin.uid == "com.incdaw.dropped");

    // Same position AND same id: an automation lane written against this slot
    // has to survive the round trip.
    CHECK(fixture.master().inserts[1].id == slotId);
}

TEST_CASE("a position past the end of the chain appends rather than failing")
{
    Fixture fixture;

    REQUIRE(fixture.registry.execute(std::make_unique<app::AddInsertCommand>(
        fixture.project.masterMixerNode(), fixture.gain(), 9)));

    REQUIRE(fixture.master().inserts.size() == 1);

    plugins::PluginIdentifier late = fixture.gain();
    late.uid = "com.incdaw.late";

    REQUIRE(fixture.registry.execute(std::make_unique<app::AddInsertCommand>(
        fixture.project.masterMixerNode(), late, 7)));

    REQUIRE(fixture.master().inserts.size() == 2);
    CHECK(fixture.master().inserts[1].plugin.uid == "com.incdaw.late");
}
