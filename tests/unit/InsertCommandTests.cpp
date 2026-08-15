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
