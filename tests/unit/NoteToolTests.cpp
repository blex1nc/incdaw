#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/NoteToolCommands.h"

#include <memory>

using namespace incdaw;
using namespace incdaw::app;

namespace {

project::MidiEvent note(Tick tick, int key, Tick duration = 240, int velocity = 100)
{
    project::MidiEvent event;
    event.type     = project::MidiEventType::note;
    event.tick     = tick;
    event.key      = key;
    event.duration = duration;
    event.value    = velocity;
    return event;
}

struct Fixture {
    project::Project project;
    CommandRegistry  registry { project };
    project::EntityId pattern;
    project::EntityId channel;

    Fixture()
    {
        channel = project.addChannel("Synth").id;
        pattern = project.addPattern("P1").id;
    }

    [[nodiscard]] std::vector<project::MidiEvent>& events()
    {
        return project.findPattern(pattern)->contentFor(channel).events;
    }
};

} // namespace

// ── Strum ─────────────────────────────────────────────────────────────────────

TEST_CASE("an upward strum staggers starts by key and anchors the ends")
{
    Fixture fixture;
    fixture.events() = { note(0, 67, 960), note(0, 60, 960), note(0, 64, 960) };

    REQUIRE(fixture.registry.execute(std::make_unique<StrumNotesCommand>(
        fixture.pattern, fixture.channel, NoteIndices { 0, 1, 2 }, 90, false)));

    // Lowest note stays; higher keys start later; every end stays at 960.
    for (const project::MidiEvent& event : fixture.events()) {
        if (event.key == 60) CHECK(event.tick == 0);
        if (event.key == 64) CHECK(event.tick == 45);
        if (event.key == 67) CHECK(event.tick == 90);
        CHECK(event.tick + event.duration == 960);
    }

    CHECK(fixture.registry.undo());
    for (const project::MidiEvent& event : fixture.events()) {
        CHECK(event.tick == 0);
        CHECK(event.duration == 960);
    }
}

TEST_CASE("a downward strum leads with the highest key")
{
    Fixture fixture;
    fixture.events() = { note(0, 60, 960), note(0, 64, 960), note(0, 67, 960) };

    REQUIRE(fixture.registry.execute(std::make_unique<StrumNotesCommand>(
        fixture.pattern, fixture.channel, NoteIndices { 0, 1, 2 }, 90, true)));

    for (const project::MidiEvent& event : fixture.events()) {
        if (event.key == 67) CHECK(event.tick == 0);
        if (event.key == 60) CHECK(event.tick == 90);
    }
}

TEST_CASE("strumming single notes changes nothing and leaves no undo entry")
{
    Fixture fixture;
    fixture.events() = { note(0, 60), note(480, 64) };

    CHECK_FALSE(fixture.registry.execute(std::make_unique<StrumNotesCommand>(
        fixture.pattern, fixture.channel, NoteIndices { 0, 1 }, 90, false)));
    CHECK_FALSE(fixture.registry.canUndo());
}

// ── Arpeggiate ────────────────────────────────────────────────────────────────

TEST_CASE("arpeggiating a chord tiles its span with cycling notes")
{
    Fixture fixture;
    fixture.events() = { note(0, 60, 960), note(0, 64, 960), note(0, 67, 960) };

    REQUIRE(fixture.registry.execute(std::make_unique<ArpeggiateNotesCommand>(
        fixture.pattern, fixture.channel, NoteIndices { 0, 1, 2 }, 240,
        ArpeggiateNotesCommand::Direction::up)));

    REQUIRE(fixture.events().size() == 4);
    CHECK(fixture.events()[0].key == 60);
    CHECK(fixture.events()[0].tick == 0);
    CHECK(fixture.events()[1].key == 64);
    CHECK(fixture.events()[1].tick == 240);
    CHECK(fixture.events()[2].key == 67);
    CHECK(fixture.events()[2].tick == 480);
    CHECK(fixture.events()[3].key == 60);
    CHECK(fixture.events()[3].tick == 720);
    for (const project::MidiEvent& event : fixture.events())
        CHECK(event.duration == 240);

    CHECK(fixture.registry.undo());
    REQUIRE(fixture.events().size() == 3);
    CHECK(fixture.events()[0].duration == 960);
}

TEST_CASE("arpeggio directions walk the chord differently")
{
    Fixture fixture;
    fixture.events() = { note(0, 60, 960), note(0, 64, 960), note(0, 67, 960) };

    SUBCASE("down starts at the top")
    {
        REQUIRE(fixture.registry.execute(std::make_unique<ArpeggiateNotesCommand>(
            fixture.pattern, fixture.channel, NoteIndices { 0, 1, 2 }, 240,
            ArpeggiateNotesCommand::Direction::down)));
        CHECK(fixture.events()[0].key == 67);
        CHECK(fixture.events()[1].key == 64);
        CHECK(fixture.events()[2].key == 60);
        CHECK(fixture.events()[3].key == 67);
    }

    SUBCASE("up-down folds back without repeating the top")
    {
        REQUIRE(fixture.registry.execute(std::make_unique<ArpeggiateNotesCommand>(
            fixture.pattern, fixture.channel, NoteIndices { 0, 1, 2 }, 240,
            ArpeggiateNotesCommand::Direction::upDown)));
        CHECK(fixture.events()[0].key == 60);
        CHECK(fixture.events()[1].key == 64);
        CHECK(fixture.events()[2].key == 67);
        CHECK(fixture.events()[3].key == 64);
    }
}

TEST_CASE("arpeggiating keeps unselected and non-chord events intact")
{
    Fixture fixture;

    project::MidiEvent controller;
    controller.type  = project::MidiEventType::controlChange;
    controller.tick  = 100;
    controller.key   = 74;
    controller.value = 90;

    fixture.events() = { note(0, 60, 480), note(0, 64, 480), controller, note(960, 72, 240) };

    REQUIRE(fixture.registry.execute(std::make_unique<ArpeggiateNotesCommand>(
        fixture.pattern, fixture.channel, NoteIndices { 0, 1 }, 240,
        ArpeggiateNotesCommand::Direction::up)));

    // 2 arpeggio notes + the CC + the untouched later note.
    REQUIRE(fixture.events().size() == 4);

    bool controllerSurvives = false;
    bool laterNoteSurvives  = false;
    for (const project::MidiEvent& event : fixture.events()) {
        if (event.type == project::MidiEventType::controlChange && event.tick == 100)
            controllerSurvives = true;
        if (event.type == project::MidiEventType::note && event.tick == 960 && event.key == 72)
            laterNoteSurvives = true;
    }
    CHECK(controllerSurvives);
    CHECK(laterNoteSurvives);
}

TEST_CASE("arpeggiating single notes is a no-op")
{
    Fixture fixture;
    fixture.events() = { note(0, 60), note(480, 64) };

    CHECK_FALSE(fixture.registry.execute(std::make_unique<ArpeggiateNotesCommand>(
        fixture.pattern, fixture.channel, NoteIndices { 0, 1 }, 240,
        ArpeggiateNotesCommand::Direction::up)));
    CHECK_FALSE(fixture.registry.canUndo());
}

// ── Legato ────────────────────────────────────────────────────────────────────

TEST_CASE("legato extends every note to the next start and spares the last")
{
    Fixture fixture;
    fixture.events() = { note(0, 60, 100), note(480, 64, 100), note(960, 67, 100) };

    REQUIRE(fixture.registry.execute(std::make_unique<LegatoNotesCommand>(
        fixture.pattern, fixture.channel, NoteIndices { 0, 1, 2 })));

    CHECK(fixture.events()[0].duration == 480);
    CHECK(fixture.events()[1].duration == 480);
    CHECK(fixture.events()[2].duration == 100);

    CHECK(fixture.registry.undo());
    CHECK(fixture.events()[0].duration == 100);
    CHECK(fixture.events()[1].duration == 100);
}

TEST_CASE("legato extends chord notes together")
{
    Fixture fixture;
    fixture.events() = { note(0, 60, 100), note(0, 64, 700), note(480, 67, 100) };

    REQUIRE(fixture.registry.execute(std::make_unique<LegatoNotesCommand>(
        fixture.pattern, fixture.channel, NoteIndices { 0, 1, 2 })));

    CHECK(fixture.events()[0].duration == 480);
    CHECK(fixture.events()[1].duration == 480);   // trimmed back to the next start
    CHECK(fixture.events()[2].duration == 100);
}

TEST_CASE("legato on an already-legato line is a no-op")
{
    Fixture fixture;
    fixture.events() = { note(0, 60, 480), note(480, 64, 100) };

    CHECK_FALSE(fixture.registry.execute(std::make_unique<LegatoNotesCommand>(
        fixture.pattern, fixture.channel, NoteIndices { 0, 1 })));
    CHECK_FALSE(fixture.registry.canUndo());
}

// ── Note labels ───────────────────────────────────────────────────────────────

TEST_CASE("labelling notes sets, undoes and redoes per note")
{
    Fixture fixture;
    fixture.events() = { note(0, 60), note(480, 64) };
    fixture.events()[0].label = "verse";

    REQUIRE(fixture.registry.execute(std::make_unique<SetNoteLabelCommand>(
        fixture.pattern, fixture.channel, NoteIndices { 0, 1 }, "hook")));

    CHECK(fixture.events()[0].label == "hook");
    CHECK(fixture.events()[1].label == "hook");

    CHECK(fixture.registry.undo());
    CHECK(fixture.events()[0].label == "verse");
    CHECK(fixture.events()[1].label.empty());

    CHECK(fixture.registry.redo());
    CHECK(fixture.events()[0].label == "hook");
}

TEST_CASE("applying the same label twice is a no-op")
{
    Fixture fixture;
    fixture.events() = { note(0, 60) };
    fixture.events()[0].label = "hook";

    CHECK_FALSE(fixture.registry.execute(std::make_unique<SetNoteLabelCommand>(
        fixture.pattern, fixture.channel, NoteIndices { 0 }, "hook")));
    CHECK_FALSE(fixture.registry.canUndo());
}
