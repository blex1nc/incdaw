#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ChannelCommands.h"
#include "app/commands/MacroCommand.h"
#include "app/commands/NoteCommands.h"

#include <memory>

using namespace incdaw;
using namespace incdaw::app;
using incdaw::engine::ticksPerQuarterNote;

namespace {

project::MidiEvent note(Tick tick, int key = 60, Tick duration = 480, int velocity = 100)
{
    project::MidiEvent event;
    event.type     = project::MidiEventType::note;
    event.tick     = tick;
    event.key      = key;
    event.duration = duration;
    event.value    = velocity;
    return event;
}

/// Records the order in which a macro ran its children, and the order in which
/// it undid them (negated, so the two are distinguishable).
class OrderCommand final : public Command {
public:
    OrderCommand(std::vector<int>& log, int mark) : log_(&log), mark_(mark) {}

    [[nodiscard]] const char* id() const noexcept override { return "test.order"; }
    [[nodiscard]] std::string name() const override { return "Order"; }

    [[nodiscard]] bool execute(project::Project&) override { log_->push_back(mark_); return true; }
    void undo(project::Project&) override { log_->push_back(-mark_); }

private:
    std::vector<int>* log_;
    int               mark_;
};

/// A project with one channel and one pattern holding `count` notes, one per
/// beat. Note edits address a (pattern, channel) pair, so a test needs both.
project::Project makeProjectWithNotes(int count, EntityId& patternId, EntityId& channelId)
{
    project::Project project;
    channelId = project.addChannel("Test Channel").id;

    auto& pattern = project.addPattern("Test");
    patternId = pattern.id;

    auto& events = pattern.contentFor(channelId).events;
    for (int index = 0; index < count; ++index)
        events.push_back(note(static_cast<Tick>(index) * ticksPerQuarterNote, 60 + index % 12));

    return project;
}

/// The notes under edit. Empty when the channel has no content yet, which is
/// what an untouched channel looks like.
const std::vector<project::MidiEvent>& notesOf(const project::Project& project, EntityId channel)
{
    static const std::vector<project::MidiEvent> empty;

    const std::vector<project::MidiEvent>* events = project.patterns()[0].events(channel);
    return events != nullptr ? *events : empty;
}

/// A trivial command, for exercising the registry itself.
class CountingCommand final : public Command {
public:
    explicit CountingCommand(int& counter, int delta = 1) : counter_(&counter), delta_(delta) {}

    [[nodiscard]] const char* id() const noexcept override { return "test.counting"; }
    [[nodiscard]] std::string name() const override { return "Count"; }

    [[nodiscard]] bool execute(project::Project&) override { *counter_ += delta_; return true; }
    void undo(project::Project&) override { *counter_ -= delta_; }

private:
    int* counter_;
    int  delta_;
};

class NoOpCommand final : public Command {
public:
    [[nodiscard]] const char* id() const noexcept override { return "test.noop"; }
    [[nodiscard]] std::string name() const override { return "Nothing"; }
    [[nodiscard]] bool execute(project::Project&) override { return false; }
    void undo(project::Project&) override {}
};

} // namespace

// ── Registry ──────────────────────────────────────────────────────────────────

TEST_CASE("executing pushes onto the undo stack and undo reverses it")
{
    project::Project project;
    CommandRegistry  registry{project};

    int counter = 0;

    CHECK(registry.execute(std::make_unique<CountingCommand>(counter)));
    CHECK(counter == 1);
    CHECK(registry.canUndo());
    CHECK(registry.undoName() == "Count");

    CHECK(registry.undo());
    CHECK(counter == 0);
    CHECK_FALSE(registry.canUndo());
    CHECK(registry.canRedo());

    CHECK(registry.redo());
    CHECK(counter == 1);
}

TEST_CASE("a command that changes nothing leaves no undo entry")
{
    // An undo entry that does nothing is worse than no entry: the user presses
    // undo, sees no change, and presses it again.
    project::Project project;
    CommandRegistry  registry{project};

    CHECK_FALSE(registry.execute(std::make_unique<NoOpCommand>()));
    CHECK_FALSE(registry.canUndo());
    CHECK(registry.undoDepth() == 0);
}

TEST_CASE("executing after undoing clears the redo stack")
{
    project::Project project;
    CommandRegistry  registry{project};

    int counter = 0;
    CHECK(registry.execute(std::make_unique<CountingCommand>(counter)));
    CHECK(registry.undo());
    REQUIRE(registry.canRedo());

    // History has diverged; the old future no longer describes this project.
    CHECK(registry.execute(std::make_unique<CountingCommand>(counter, 10)));
    CHECK_FALSE(registry.canRedo());
}

TEST_CASE("undo and redo survive many round trips")
{
    project::Project project;
    CommandRegistry  registry{project};

    int counter = 0;
    for (int index = 0; index < 50; ++index)
        CHECK(registry.execute(std::make_unique<CountingCommand>(counter)));

    CHECK(counter == 50);

    for (int index = 0; index < 50; ++index)
        CHECK(registry.undo());

    CHECK(counter == 0);

    for (int index = 0; index < 50; ++index)
        CHECK(registry.redo());

    CHECK(counter == 50);
}

TEST_CASE("history is bounded so a long session cannot grow without limit")
{
    project::Project project;
    CommandRegistry  registry{project};
    registry.setMaximumDepth(10);

    int counter = 0;
    for (int index = 0; index < 100; ++index)
        CHECK(registry.execute(std::make_unique<CountingCommand>(counter)));

    CHECK(registry.undoDepth() == 10);
}

TEST_CASE("actions are addressable by id, which is what shortcuts and scripting need")
{
    project::Project project;
    CommandRegistry  registry{project};

    int counter = 0;
    registry.registerAction({"test.counting", "Count Up", "Test", "Cmd+K",
                             [&counter] { return std::make_unique<CountingCommand>(counter); }});

    REQUIRE(registry.findAction("test.counting") != nullptr);
    CHECK(registry.findAction("test.counting")->defaultShortcut == "Cmd+K");

    CHECK(registry.invoke("test.counting"));
    CHECK(counter == 1);

    CHECK_FALSE(registry.invoke("test.doesNotExist"));
}

TEST_CASE("re-registering an id replaces it rather than shadowing it")
{
    project::Project project;
    CommandRegistry  registry{project};

    int first = 0;
    int second = 0;

    registry.registerAction({"a", "First", "Test", "", [&first] { return std::make_unique<CountingCommand>(first); }});
    registry.registerAction({"a", "Second", "Test", "", [&second] { return std::make_unique<CountingCommand>(second); }});

    CHECK(registry.actions().size() == 1);
    CHECK(registry.invoke("a"));
    CHECK(first == 0);
    CHECK(second == 1);
}

TEST_CASE("command search finds actions by name, id and category")
{
    project::Project project;
    CommandRegistry  registry{project};

    int counter = 0;
    const auto make = [&counter] { return std::make_unique<CountingCommand>(counter); };

    registry.registerAction({"pianoroll.quantize", "Quantize Notes", "Piano Roll", "", make});
    registry.registerAction({"transport.play", "Play", "Transport", "", make});
    registry.registerAction({"mixer.mute", "Mute Track", "Mixer", "", make});

    CHECK(registry.search("quant").size() == 1);
    CHECK(registry.search("QUANT").size() == 1);       // case-insensitive
    CHECK(registry.search("transport").size() == 1);   // by id and category
    CHECK(registry.search("").size() == 3);
    CHECK(registry.search("nothing here").empty());
}

// ── Note commands ─────────────────────────────────────────────────────────────

TEST_CASE("adding a note is undoable")
{
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(0, patternId, channelId);
    CommandRegistry  registry{project};

    CHECK(registry.execute(std::make_unique<AddNoteCommand>(patternId, channelId, note(0, 64))));
    REQUIRE(notesOf(project, channelId).size() == 1);
    CHECK(notesOf(project, channelId)[0].key == 64);

    CHECK(registry.undo());
    CHECK(notesOf(project, channelId).empty());

    CHECK(registry.redo());
    CHECK(notesOf(project, channelId).size() == 1);
}

TEST_CASE("deleting notes restores them in their original positions")
{
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(6, patternId, channelId);
    CommandRegistry  registry{project};

    const auto before = notesOf(project, channelId);

    CHECK(registry.execute(std::make_unique<DeleteNotesCommand>(patternId, channelId, NoteIndices{1, 3, 4})));
    CHECK(notesOf(project, channelId).size() == 3);

    CHECK(registry.undo());
    CHECK(notesOf(project, channelId) == before);
}

TEST_CASE("deleting nothing is not an undo entry")
{
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(2, patternId, channelId);
    CommandRegistry  registry{project};

    // Stale indices: the selection outlived the notes it referred to.
    CHECK_FALSE(registry.execute(std::make_unique<DeleteNotesCommand>(patternId, channelId, NoteIndices{99, 100})));
    CHECK(notesOf(project, channelId).size() == 2);
    CHECK(registry.undoDepth() == 0);
}

TEST_CASE("moving notes is exactly reversible")
{
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(4, patternId, channelId);
    CommandRegistry  registry{project};

    const auto before = notesOf(project, channelId);

    CHECK(registry.execute(std::make_unique<MoveNotesCommand>(patternId, channelId, NoteIndices{0, 1, 2, 3}, 240, 5)));

    CHECK(notesOf(project, channelId)[0].tick == 240);
    CHECK(notesOf(project, channelId)[0].key == before[0].key + 5);

    CHECK(registry.undo());
    CHECK(notesOf(project, channelId) == before);
}

TEST_CASE("a selection dragged to the edge keeps its shape")
{
    // Clamping each note independently would flatten a chord against the edge
    // of the keyboard; the selection must move as a unit.
    EntityId patternId;
    EntityId channelId;
    project::Project project;
    channelId = project.addChannel("Channel").id;

    auto& pattern = project.addPattern("Chord");
    patternId = pattern.id;

    auto& events = pattern.contentFor(channelId).events;

    events.push_back(note(0, 0));    // already at the lowest key
    events.push_back(note(0, 7));
    events.push_back(note(0, 12));

    CommandRegistry registry{project};

    CHECK_FALSE(registry.execute(std::make_unique<MoveNotesCommand>(patternId, channelId, NoteIndices{0, 1, 2}, 0, -5)));

    // Nothing could move, so nothing moved — the interval structure is intact.
    CHECK(events[0].key == 0);
    CHECK(events[1].key == 7);
    CHECK(events[2].key == 12);
}

TEST_CASE("a selection dragged left stops at the pattern start as a unit")
{
    // Clamping each note independently would compress the rhythm against tick
    // zero. The whole selection stops when its earliest note reaches the start,
    // so the spacing between notes is preserved.
    EntityId patternId;
    EntityId channelId;
    project::Project project;
    channelId = project.addChannel("Channel").id;

    auto& pattern = project.addPattern("Phrase");
    patternId = pattern.id;

    auto& events = pattern.contentFor(channelId).events;

    events.push_back(note(480));
    events.push_back(note(1440));

    CommandRegistry registry{project};

    CHECK(registry.execute(std::make_unique<MoveNotesCommand>(patternId, channelId, NoteIndices{0, 1}, -100000, 0)));

    CHECK(events[0].tick == 0);
    CHECK(events[1].tick == 960);   // spacing of 960 intact

    CHECK(registry.undo());
    CHECK(events[0].tick == 480);
    CHECK(events[1].tick == 1440);
}

TEST_CASE("a selection already at the start does not move, and is not an undo entry")
{
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(2, patternId, channelId);
    CommandRegistry  registry{project};

    CHECK_FALSE(registry.execute(std::make_unique<MoveNotesCommand>(patternId, channelId, NoteIndices{0, 1}, -100000, 0)));

    CHECK(notesOf(project, channelId)[0].tick == 0);
    CHECK(notesOf(project, channelId)[1].tick == ticksPerQuarterNote);
    CHECK(registry.undoDepth() == 0);
}

TEST_CASE("a drag gesture collapses into one undo step")
{
    // Without merging, reversing a single drag would take dozens of undos.
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(2, patternId, channelId);
    CommandRegistry  registry{project};

    const auto before = notesOf(project, channelId);

    for (int step = 0; step < 20; ++step)
        CHECK(registry.executeMerging(
            std::make_unique<MoveNotesCommand>(patternId, channelId, NoteIndices{0, 1}, 10, 0)));

    CHECK(registry.undoDepth() == 1);
    CHECK(notesOf(project, channelId)[0].tick == before[0].tick + 200);

    CHECK(registry.undo());
    CHECK(notesOf(project, channelId) == before);
}

TEST_CASE("drags on different selections do not merge")
{
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(4, patternId, channelId);
    CommandRegistry  registry{project};

    CHECK(registry.executeMerging(std::make_unique<MoveNotesCommand>(patternId, channelId, NoteIndices{0}, 10, 0)));
    CHECK(registry.executeMerging(std::make_unique<MoveNotesCommand>(patternId, channelId, NoteIndices{1}, 10, 0)));

    CHECK(registry.undoDepth() == 2);
}

TEST_CASE("resizing is reversible and cannot make a note vanish")
{
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(3, patternId, channelId);
    CommandRegistry  registry{project};

    const auto before = notesOf(project, channelId);

    CHECK(registry.execute(std::make_unique<ResizeNotesCommand>(patternId, channelId, NoteIndices{0, 1, 2}, -100000)));

    for (const auto& event : notesOf(project, channelId))
        CHECK(event.duration >= 1);

    CHECK(registry.undo());
    CHECK(notesOf(project, channelId) == before);
}

TEST_CASE("a resize gesture merges but still undoes to the original lengths")
{
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(1, patternId, channelId);
    CommandRegistry  registry{project};

    const auto before = notesOf(project, channelId);

    for (int step = 0; step < 10; ++step)
        CHECK(registry.executeMerging(
            std::make_unique<ResizeNotesCommand>(patternId, channelId, NoteIndices{0}, 20)));

    CHECK(registry.undoDepth() == 1);
    CHECK(registry.undo());
    CHECK(notesOf(project, channelId) == before);
}

TEST_CASE("velocity never reaches zero, which would silence the note")
{
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(2, patternId, channelId);
    CommandRegistry  registry{project};

    CHECK(registry.execute(std::make_unique<SetVelocityCommand>(patternId, channelId, NoteIndices{0, 1}, 0)));

    for (const auto& event : notesOf(project, channelId))
        CHECK(event.value >= 1);
}

TEST_CASE("quantize is undoable and does nothing twice")
{
    EntityId patternId;
    EntityId channelId;
    project::Project project;
    channelId = project.addChannel("Channel").id;

    auto& pattern = project.addPattern("Loose");
    patternId = pattern.id;

    auto& events = pattern.contentFor(channelId).events;

    for (const Tick tick : {13, 231, 462, 701})
        events.push_back(note(tick));

    CommandRegistry registry{project};
    const auto before = events;

    const Tick grid = ticksPerQuarterNote / 4;

    CHECK(registry.execute(std::make_unique<QuantizeNotesCommand>(patternId, channelId, grid, 1.0)));
    for (const auto& event : notesOf(project, channelId))
        CHECK(event.tick % grid == 0);

    // Already on the grid: no change, so no undo entry.
    CHECK_FALSE(registry.execute(std::make_unique<QuantizeNotesCommand>(patternId, channelId, grid, 1.0)));
    CHECK(registry.undoDepth() == 1);

    CHECK(registry.undo());
    CHECK(notesOf(project, channelId) == before);
}

TEST_CASE("interleaved edits undo back to the exact starting state")
{
    // The property that matters most: any sequence of commands, fully undone,
    // must restore the project bit for bit.
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(8, patternId, channelId);
    CommandRegistry  registry{project};

    const auto before = notesOf(project, channelId);

    CHECK(registry.execute(std::make_unique<AddNoteCommand>(patternId, channelId, note(5000, 72))));
    CHECK(registry.execute(std::make_unique<MoveNotesCommand>(patternId, channelId, NoteIndices{0, 2, 4}, 120, 2)));
    CHECK(registry.execute(std::make_unique<ResizeNotesCommand>(patternId, channelId, NoteIndices{1, 3}, 200)));
    CHECK(registry.execute(std::make_unique<SetVelocityCommand>(patternId, channelId, NoteIndices{5, 6}, 40)));
    CHECK(registry.execute(std::make_unique<DeleteNotesCommand>(patternId, channelId, NoteIndices{7})));
    CHECK(registry.execute(std::make_unique<QuantizeNotesCommand>(patternId, channelId, ticksPerQuarterNote / 3, 0.8)));

    while (registry.canUndo())
        CHECK(registry.undo());

    CHECK(notesOf(project, channelId) == before);
}

TEST_CASE("redoing an interleaved sequence reproduces the same result")
{
    EntityId patternId;
    EntityId channelId;
    project::Project project = makeProjectWithNotes(8, patternId, channelId);
    CommandRegistry  registry{project};

    CHECK(registry.execute(std::make_unique<AddNoteCommand>(patternId, channelId, note(5000, 72))));
    CHECK(registry.execute(std::make_unique<MoveNotesCommand>(patternId, channelId, NoteIndices{0, 2}, 120, 2)));
    CHECK(registry.execute(std::make_unique<SetVelocityCommand>(patternId, channelId, NoteIndices{1}, 40)));

    const auto after = notesOf(project, channelId);

    while (registry.canUndo())
        CHECK(registry.undo());

    while (registry.canRedo())
        CHECK(registry.redo());

    CHECK(notesOf(project, channelId) == after);
}

TEST_CASE("commands targeting a pattern that no longer exists fail safely")
{
    project::Project project;
    CommandRegistry  registry{project};

    const EntityId missing{9999};

    CHECK_FALSE(registry.execute(std::make_unique<AddNoteCommand>(missing, missing, note(0))));
    CHECK_FALSE(registry.execute(std::make_unique<DeleteNotesCommand>(missing, missing,
                                                                     NoteIndices{0})));
    CHECK_FALSE(registry.execute(std::make_unique<MoveNotesCommand>(missing, missing,
                                                                   NoteIndices{0}, 10, 0)));
    CHECK_FALSE(registry.execute(std::make_unique<QuantizeNotesCommand>(missing, missing, 240,
                                                                       1.0)));

    CHECK(registry.undoDepth() == 0);
}

// ── Macros: one gesture, one undo entry ──────────────────────────────────────

TEST_CASE("a macro executes its children in order and undoes them in reverse")
{
    project::Project project;

    std::vector<int> order;

    auto macro = std::make_unique<MacroCommand>("test.macro", "Test Macro");
    macro->add(std::make_unique<OrderCommand>(order, 1));
    macro->add(std::make_unique<OrderCommand>(order, 2));
    macro->add(std::make_unique<OrderCommand>(order, 3));

    CHECK(macro->execute(project));
    CHECK(order == std::vector<int>{1, 2, 3});

    order.clear();
    macro->undo(project);
    CHECK(order == std::vector<int>{-3, -2, -1});
}

TEST_CASE("a macro is one undo entry, named once")
{
    project::Project project;
    CommandRegistry  registry{project};

    auto macro = std::make_unique<MacroCommand>("channel.dropSample", "Load Sample");
    macro->add(std::make_unique<AddChannelCommand>("Kick"));
    macro->add(std::make_unique<AddChannelCommand>("Snare"));

    REQUIRE(registry.execute(std::move(macro)));
    CHECK(project.channels().size() == 2);
    CHECK(registry.undoDepth() == 1);
    CHECK(registry.undoName() == "Load Sample");   // the entry names the gesture, once

    CHECK(registry.undo());
    CHECK(project.channels().empty());

    CHECK(registry.redo());
    CHECK(project.channels().size() == 2);
}

TEST_CASE("a later step can target what an earlier one minted")
{
    // The drop gesture: add a channel, then act on the channel that command
    // created. The id does not exist until the first child has run.
    project::Project project;
    CommandRegistry  registry{project};

    auto add    = std::make_unique<AddChannelCommand>("Dropped");
    auto* added = add.get();

    auto macro = std::make_unique<MacroCommand>("test.mint", "Drop");
    macro->add(std::move(add));
    macro->addStep([added](project::Project&) -> CommandPtr {
        return std::make_unique<RenameChannelCommand>(added->channelId(), "Renamed");
    });

    REQUIRE(registry.execute(std::move(macro)));
    REQUIRE(project.channels().size() == 1);
    CHECK(project.channels()[0].name == "Renamed");

    CHECK(registry.undo());
    CHECK(project.channels().empty());

    // Redo replays the children rather than rebuilding them, so the second
    // child still addresses the channel the first one restored.
    CHECK(registry.redo());
    REQUIRE(project.channels().size() == 1);
    CHECK(project.channels()[0].name == "Renamed");
}

TEST_CASE("a macro of no-ops is itself a no-op")
{
    project::Project project;
    CommandRegistry  registry{project};

    auto empty = std::make_unique<MacroCommand>("test.empty", "Nothing");
    CHECK_FALSE(empty->execute(project));

    auto noops = std::make_unique<MacroCommand>("test.noops", "Nothing");
    noops->addStep([](project::Project&) -> CommandPtr { return nullptr; });
    noops->add(std::make_unique<RenameChannelCommand>(EntityId{}, "ghost"));   // no such channel

    CHECK_FALSE(registry.execute(std::move(noops)));
    CHECK(registry.undoDepth() == 0);
}

TEST_CASE("children that did nothing are dropped rather than replayed")
{
    project::Project project;
    const EntityId   channelId = project.addChannel("Kick").id;

    auto macro = std::make_unique<MacroCommand>("test.mixed", "Mixed");
    macro->add(std::make_unique<RenameChannelCommand>(channelId, "Kick"));      // same name: a no-op
    macro->add(std::make_unique<RenameChannelCommand>(channelId, "Kick 2"));

    CHECK(macro->execute(project));
    CHECK(macro->childCount() == 1);
    CHECK(project.channels()[0].name == "Kick 2");

    macro->undo(project);
    CHECK(project.channels()[0].name == "Kick");
}
