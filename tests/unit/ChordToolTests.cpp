#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/MusicTheory.h"
#include "app/commands/ChordCommands.h"

#include <memory>

using namespace incdaw;
using namespace incdaw::app;
namespace music = incdaw::app::music;

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

    [[nodiscard]] std::vector<int> keys()
    {
        std::vector<int> result;
        for (const project::MidiEvent& event : events())
            result.push_back(event.key);
        return result;
    }
};

} // namespace

// ── Note names ────────────────────────────────────────────────────────────────

TEST_CASE("note names use the C4-is-middle-C convention")
{
    CHECK(music::noteName(60) == "C4");
    CHECK(music::noteName(61) == "C#4");
    CHECK(music::noteName(69) == "A4");
    CHECK(music::noteName(0) == "C-1");
}

// ── Detection ─────────────────────────────────────────────────────────────────

TEST_CASE("a root-position major triad is detected with its plain name")
{
    const music::ChordDetection detection = music::detectChord({ 60, 64, 67 });
    CHECK(detection.matched);
    CHECK(detection.rootPitchClass == 0);
    CHECK_FALSE(detection.inverted);
    CHECK(detection.display == "C");
}

TEST_CASE("an inversion is detected and displayed with its bass")
{
    const music::ChordDetection detection = music::detectChord({ 64, 67, 72 });
    CHECK(detection.matched);
    CHECK(detection.rootPitchClass == 0);
    CHECK(detection.inverted);
    CHECK(detection.display == "C/E");
}

TEST_CASE("sevenths are detected regardless of input order")
{
    const music::ChordDetection detection = music::detectChord({ 67, 57, 64, 60 });
    CHECK(detection.matched);
    CHECK(detection.display == "Am7");
}

TEST_CASE("one key reads as a note name, two as an interval")
{
    CHECK(music::detectChord({ 60 }).display == "C4");
    CHECK(music::detectChord({ 60, 67 }).display == "C4–G4 (P5)");
    CHECK_FALSE(music::detectChord({ 60, 67 }).matched);
}

TEST_CASE("the shared m6 / m7b5 pitch-class set is named from its bass")
{
    // C Eb G A from C is Cm6; the same classes from A are Am7b5.
    CHECK(music::detectChord({ 60, 63, 67, 69 }).display == "Cm6");
    CHECK(music::detectChord({ 57, 60, 63, 67 }).display == "Am7b5");
}

TEST_CASE("a chromatic cluster does not match")
{
    const music::ChordDetection detection = music::detectChord({ 60, 61, 62 });
    CHECK_FALSE(detection.matched);
    CHECK(detection.display.empty());
}

// ── Stamping ──────────────────────────────────────────────────────────────────

TEST_CASE("bottom-up stamping builds upward from the root")
{
    const music::ChordType* major = music::findChordType("");
    REQUIRE(major != nullptr);
    CHECK(music::stampChord(60, *major, music::StampVoicing::bottomUp)
          == std::vector<int> { 60, 64, 67 });

    const music::ChordType* add9 = music::findChordType("add9");
    REQUIRE(add9 != nullptr);
    CHECK(music::stampChord(60, *add9, music::StampVoicing::bottomUp)
          == std::vector<int> { 60, 64, 67, 74 });
}

TEST_CASE("top-down stamping keeps the clicked key as the root on top")
{
    const music::ChordType* major = music::findChordType("");
    REQUIRE(major != nullptr);
    const std::vector<int> keys = music::stampChord(60, *major, music::StampVoicing::topDown);
    CHECK(keys == std::vector<int> { 52, 55, 60 });

    // Still detects as C major, first inversion.
    CHECK(music::detectChord(keys).display == "C/E");
}

TEST_CASE("stamping at the keyboard edge drops what does not fit")
{
    const music::ChordType* major = music::findChordType("");
    REQUIRE(major != nullptr);
    CHECK(music::stampChord(126, *major, music::StampVoicing::bottomUp)
          == std::vector<int> { 126 });
}

// ── Voice-leading ─────────────────────────────────────────────────────────────

TEST_CASE("voice-leading from C to A minor keeps the common tones")
{
    const std::vector<int> led = music::voiceLead({ 60, 64, 67 }, { 9, 0, 4 });
    CHECK(led == std::vector<int> { 60, 64, 69 });
}

TEST_CASE("voice-leading from C to G chooses the near inversion")
{
    const std::vector<int> led = music::voiceLead({ 60, 64, 67 }, { 7, 11, 2 });
    CHECK(led == std::vector<int> { 59, 62, 67 });
}

// ── Scales ────────────────────────────────────────────────────────────────────

TEST_CASE("diatonic chords stack thirds inside the scale")
{
    using music::Scale;
    CHECK(music::diatonicChordPitchClasses(0, Scale::major, 0, 3)
          == std::vector<int> { 0, 4, 7 });
    CHECK(music::diatonicChordPitchClasses(0, Scale::major, 5, 3)
          == std::vector<int> { 9, 0, 4 });
    CHECK(music::diatonicChordPitchClasses(0, Scale::major, 4, 4)
          == std::vector<int> { 7, 11, 2, 5 });
}

TEST_CASE("degree lookups handle diatonic and chromatic pitch classes")
{
    using music::Scale;
    CHECK(music::degreeOf(0, Scale::major, 7) == 4);
    CHECK(music::degreeOf(0, Scale::major, 6) == -1);
    CHECK(music::nearestDegree(0, Scale::major, 6) == 3);
}

// ── InsertNotesCommand ────────────────────────────────────────────────────────

TEST_CASE("a stamped chord lands as one undo step")
{
    Fixture fixture;

    const music::ChordType* major = music::findChordType("");
    REQUIRE(major != nullptr);

    std::vector<project::MidiEvent> notes;
    for (const int key : music::stampChord(60, *major, music::StampVoicing::bottomUp))
        notes.push_back(note(0, key, 480));

    REQUIRE(fixture.registry.execute(std::make_unique<InsertNotesCommand>(
        fixture.pattern, fixture.channel, notes, "Stamp Chord")));

    CHECK(fixture.events().size() == 3);
    CHECK(fixture.registry.undoName() == "Stamp Chord");

    CHECK(fixture.registry.undo());
    CHECK(fixture.events().empty());

    CHECK(fixture.registry.redo());
    CHECK(fixture.keys() == std::vector<int> { 60, 64, 67 });
}

TEST_CASE("inserting only out-of-range notes is a no-op")
{
    Fixture fixture;

    REQUIRE_FALSE(fixture.registry.execute(std::make_unique<InsertNotesCommand>(
        fixture.pattern, fixture.channel,
        std::vector<project::MidiEvent> { note(0, 200) }, "Stamp Chord")));
    CHECK_FALSE(fixture.registry.canUndo());
    CHECK(fixture.events().empty());
}

// ── NudgeChordCommand ─────────────────────────────────────────────────────────

TEST_CASE("nudging a C major triad up a degree yields a voice-led D minor")
{
    Fixture fixture;
    fixture.events() = { note(0, 60), note(0, 64), note(0, 67) };

    REQUIRE(fixture.registry.execute(std::make_unique<NudgeChordCommand>(
        fixture.pattern, fixture.channel, std::vector<std::size_t> { 0, 1, 2 },
        0, music::Scale::major, 1)));

    CHECK(fixture.keys() == std::vector<int> { 62, 65, 69 });
    CHECK(music::detectChord(fixture.keys()).display == "Dm");

    CHECK(fixture.registry.undo());
    CHECK(fixture.keys() == std::vector<int> { 60, 64, 67 });
}

TEST_CASE("merged nudges walk the progression but undo in one step")
{
    Fixture fixture;
    fixture.events() = { note(0, 60), note(0, 64), note(0, 67) };

    REQUIRE(fixture.registry.executeMerging(std::make_unique<NudgeChordCommand>(
        fixture.pattern, fixture.channel, std::vector<std::size_t> { 0, 1, 2 },
        0, music::Scale::major, 1)));
    REQUIRE(fixture.registry.executeMerging(std::make_unique<NudgeChordCommand>(
        fixture.pattern, fixture.channel, std::vector<std::size_t> { 0, 1, 2 },
        0, music::Scale::major, 1)));

    // C → Dm → Em, voice-led from the Dm voicing.
    CHECK(music::detectChord(fixture.keys()).display == "Em");
    CHECK(fixture.registry.undoDepth() == 1);

    CHECK(fixture.registry.undo());
    CHECK(fixture.keys() == std::vector<int> { 60, 64, 67 });
    CHECK_FALSE(fixture.registry.canUndo());
}

TEST_CASE("a nudge rewrites keys only and leaves other events alone")
{
    Fixture fixture;

    project::MidiEvent controller;
    controller.type = project::MidiEventType::controlChange;
    controller.key  = 74;
    controller.value = 90;

    project::MidiEvent labelled = note(0, 60, 480, 87);
    labelled.label              = "hook";

    fixture.events() = { labelled, note(0, 64), note(0, 67), controller };

    REQUIRE(fixture.registry.execute(std::make_unique<NudgeChordCommand>(
        fixture.pattern, fixture.channel, std::vector<std::size_t> { 0, 1, 2, 3 },
        0, music::Scale::major, 1)));

    CHECK(fixture.events()[0].label == "hook");
    CHECK(fixture.events()[0].value == 87);
    CHECK(fixture.events()[0].duration == 480);
    CHECK(fixture.events()[3].key == 74);      // the CC was not part of the chord
    CHECK(fixture.events()[3].value == 90);
}

TEST_CASE("doubled keys survive a nudge through nearest-tone mapping")
{
    Fixture fixture;
    fixture.events() = { note(0, 60), note(0, 60), note(0, 64), note(0, 67) };

    REQUIRE(fixture.registry.execute(std::make_unique<NudgeChordCommand>(
        fixture.pattern, fixture.channel, std::vector<std::size_t> { 0, 1, 2, 3 },
        0, music::Scale::major, 1)));

    CHECK(fixture.keys() == std::vector<int> { 62, 62, 65, 69 });
}
