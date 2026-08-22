// Phase 8b — the Channel Rack, the pattern list, and the step sequencer.
//
// Two properties carry this phase. First, every rack and pattern edit is a
// command, so undo returns the project to exactly what it was — compared here
// with Project's own operator==, not field by field. Second, a step IS a note:
// programming a step produces an ordinary MidiEvent that the Piano Roll can see
// and edit, because there is no second representation for the two editors to
// disagree about.

#include "doctest.h"

#include "app/ChannelRackModel.h"
#include "app/CommandRegistry.h"
#include "app/PianoRollModel.h"
#include "app/commands/ChannelCommands.h"
#include "app/commands/NoteCommands.h"
#include "app/commands/PatternCommands.h"
#include "app/commands/StepCommands.h"
#include "engine/transport/TempoMap.h"
#include "project/ProjectFile.h"
#include "project/ProjectGraphCompiler.h"

#include <filesystem>
#include <memory>
#include <vector>

using namespace incdaw;
using incdaw::engine::Tick;
using incdaw::engine::ticksPerQuarterNote;

namespace {

constexpr Tick step16 = ticksPerQuarterNote / 4;

project::MidiEvent note(Tick tick, int key = 60)
{
    project::MidiEvent event;
    event.type     = project::MidiEventType::note;
    event.tick     = tick;
    event.duration = step16;
    event.key      = key;
    event.value    = 100;
    return event;
}

app::ToggleStepCommand::Step stepAt(project::EntityId pattern, project::EntityId channel,
                                    int index, int key = 60)
{
    app::ToggleStepCommand::Step step;
    step.pattern  = pattern;
    step.channel  = channel;
    step.start    = static_cast<Tick>(index) * step16;
    step.length   = step16;
    step.key      = key;
    step.velocity = 100;
    return step;
}

} // namespace

// ── Channel commands ──────────────────────────────────────────────────────────

TEST_CASE("adding a channel is undoable and keeps its id across redo")
{
    project::Project project;
    app::CommandRegistry registry{project};

    const project::Project original = project;

    auto command = std::make_unique<app::AddChannelCommand>("Bass");
    app::AddChannelCommand* raw = command.get();

    REQUIRE(registry.execute(std::move(command)));
    REQUIRE(project.channels().size() == 1);
    CHECK(project.channels().front().name == "Bass");

    const project::EntityId minted = raw->channelId();
    CHECK(minted.isValid());

    REQUIRE(registry.undo());
    CHECK(project.channels().empty());
    CHECK(project == original);

    REQUIRE(registry.redo());
    REQUIRE(project.channels().size() == 1);

    // The id must survive the round trip. A fresh id on redo would orphan every
    // reference to the channel — pattern content above all.
    CHECK(project.channels().front().id == minted);
}

TEST_CASE("removing a channel takes its pattern content with it, and gives it back")
{
    project::Project project;
    app::CommandRegistry registry{project};

    const project::EntityId kick  = project.addChannel("Kick").id;
    const project::EntityId snare = project.addChannel("Snare").id;

    // By id, not by reference: addPattern can reallocate the vector, and a
    // reference taken before the second call would dangle.
    const project::EntityId one = project.addPattern("One").id;
    const project::EntityId two = project.addPattern("Two").id;

    project.findPattern(one)->contentFor(kick).events.push_back(note(0));
    project.findPattern(one)->contentFor(snare).events.push_back(note(step16 * 4));
    project.findPattern(two)->contentFor(kick).events.push_back(note(step16 * 8));

    const project::Project original = project;

    REQUIRE(registry.execute(std::make_unique<app::RemoveChannelCommand>(kick)));

    CHECK(project.channels().size() == 1);
    CHECK(project.findChannel(kick) == nullptr);
    CHECK(project.patterns()[0].content(kick) == nullptr);
    CHECK(project.patterns()[1].content(kick) == nullptr);

    // The other channel's content is untouched.
    REQUIRE(project.patterns()[0].content(snare) != nullptr);
    CHECK(project.patterns()[0].content(snare)->events.size() == 1);

    REQUIRE(registry.undo());
    CHECK(project == original);
}

TEST_CASE("rename, mute, solo and volume round trip")
{
    project::Project project;
    app::CommandRegistry registry{project};

    const project::EntityId channel = project.addChannel("Channel 1").id;
    const project::Project original = project;

    REQUIRE(registry.execute(std::make_unique<app::RenameChannelCommand>(channel, "Lead")));
    CHECK(project.findChannel(channel)->name == "Lead");

    REQUIRE(registry.execute(std::make_unique<app::SetChannelMutedCommand>(channel, true)));
    CHECK(project.findChannel(channel)->muted);

    REQUIRE(registry.execute(std::make_unique<app::SetChannelSoloedCommand>(channel, true)));
    CHECK(project.findChannel(channel)->soloed);

    REQUIRE(registry.execute(std::make_unique<app::SetChannelVolumeCommand>(channel, 0.25)));
    CHECK(project.findChannel(channel)->volume == doctest::Approx(0.25));

    while (registry.canUndo())
        REQUIRE(registry.undo());

    CHECK(project == original);
}

TEST_CASE("a command that changes nothing leaves no undo entry")
{
    project::Project project;
    app::CommandRegistry registry{project};

    const project::EntityId channel = project.addChannel("Channel 1").id;

    CHECK_FALSE(registry.execute(std::make_unique<app::RenameChannelCommand>(channel, "Channel 1")));
    CHECK_FALSE(registry.execute(std::make_unique<app::SetChannelMutedCommand>(channel, false)));
    CHECK(registry.undoDepth() == 0);
}

TEST_CASE("dragging a channel fader is one undo")
{
    project::Project project;
    app::CommandRegistry registry{project};

    const project::EntityId channel = project.addChannel("Channel 1").id;

    for (const double volume : {0.9, 0.8, 0.7, 0.6})
        REQUIRE(registry.executeMerging(std::make_unique<app::SetChannelVolumeCommand>(channel, volume)));

    CHECK(registry.undoDepth() == 1);
    CHECK(project.findChannel(channel)->volume == doctest::Approx(0.6));

    REQUIRE(registry.undo());
    CHECK(project.findChannel(channel)->volume == doctest::Approx(1.0));
}

// ── Mute and solo reach the audio graph ───────────────────────────────────────

TEST_CASE("mute and solo decide which channels are compiled into the graph")
{
    project::Project project;
    app::CommandRegistry registry{project};

    const project::EntityId a = project.addChannel("A").id;
    const project::EntityId b = project.addChannel("B").id;

    project::Pattern& pattern = project.addPattern("Pattern 1");
    pattern.contentFor(a).events.push_back(note(0));
    pattern.contentFor(b).events.push_back(note(0, 64));

    engine::TempoMap tempo;
    tempo.setSampleRate(48000.0);

    project::GraphCompileOptions options;
    options.pattern = pattern.id;

    const auto all = project::compileProjectGraph(project, tempo, options);
    REQUIRE(all);
    CHECK(all.channels.size() == 2);

    REQUIRE(registry.execute(std::make_unique<app::SetChannelMutedCommand>(a, true)));
    const auto muted = project::compileProjectGraph(project, tempo, options);
    REQUIRE(muted);
    CHECK(muted.channels.size() == 1);
    CHECK(muted.instrumentFor(a) == nullptr);
    CHECK(muted.instrumentFor(b) != nullptr);

    REQUIRE(registry.undo());
    REQUIRE(registry.execute(std::make_unique<app::SetChannelSoloedCommand>(a, true)));

    const auto soloed = project::compileProjectGraph(project, tempo, options);
    REQUIRE(soloed);
    CHECK(soloed.channels.size() == 1);
    CHECK(soloed.instrumentFor(a) != nullptr);
    CHECK(soloed.instrumentFor(b) == nullptr);
}

// ── Pattern commands ──────────────────────────────────────────────────────────

TEST_CASE("pattern add, duplicate, rename, length, swing and remove all round trip")
{
    project::Project project;
    app::CommandRegistry registry{project};

    const project::EntityId channel = project.addChannel("Channel 1").id;
    const project::Project original = project;

    auto add = std::make_unique<app::AddPatternCommand>("Pattern 1");
    app::AddPatternCommand* rawAdd = add.get();
    REQUIRE(registry.execute(std::move(add)));

    const project::EntityId first = rawAdd->patternId();
    project.findPattern(first)->contentFor(channel).events.push_back(note(0));

    auto duplicate = std::make_unique<app::DuplicatePatternCommand>(first, "Pattern 2");
    app::DuplicatePatternCommand* rawDuplicate = duplicate.get();
    REQUIRE(registry.execute(std::move(duplicate)));

    const project::EntityId copy = rawDuplicate->patternId();
    REQUIRE(copy != first);

    // A duplicate is a copy: the content came with it, and editing one does not
    // touch the other.
    REQUIRE(project.findPattern(copy)->content(channel) != nullptr);
    CHECK(project.findPattern(copy)->content(channel)->events.size() == 1);

    project.findPattern(copy)->contentFor(channel).events.push_back(note(step16));
    CHECK(project.findPattern(first)->content(channel)->events.size() == 1);

    REQUIRE(registry.execute(std::make_unique<app::RenamePatternCommand>(copy, "Chorus")));
    CHECK(project.findPattern(copy)->name == "Chorus");

    REQUIRE(registry.execute(
        std::make_unique<app::SetPatternLengthCommand>(copy, ticksPerQuarterNote * 8)));
    CHECK(project.findPattern(copy)->length == ticksPerQuarterNote * 8);

    REQUIRE(registry.execute(std::make_unique<app::SetPatternSwingCommand>(copy, 0.4)));
    CHECK(project.findPattern(copy)->swing == doctest::Approx(0.4));

    REQUIRE(registry.execute(std::make_unique<app::RemovePatternCommand>(first)));
    CHECK(project.findPattern(first) == nullptr);

    while (registry.canUndo())
        REQUIRE(registry.undo());

    CHECK(project == original);
}

TEST_CASE("a removed pattern comes back in its original position")
{
    project::Project project;
    app::CommandRegistry registry{project};

    const project::EntityId a = project.addPattern("A").id;
    const project::EntityId b = project.addPattern("B").id;
    const project::EntityId c = project.addPattern("C").id;

    REQUIRE(registry.execute(std::make_unique<app::RemovePatternCommand>(b)));
    REQUIRE(project.patterns().size() == 2);

    REQUIRE(registry.undo());
    REQUIRE(project.patterns().size() == 3);
    CHECK(project.patterns()[0].id == a);
    CHECK(project.patterns()[1].id == b);
    CHECK(project.patterns()[2].id == c);
}

// ── Steps are notes ───────────────────────────────────────────────────────────

TEST_CASE("toggling a step writes an ordinary note the Piano Roll can see")
{
    project::Project project;
    app::CommandRegistry registry{project};

    project::Channel& channel = project.addChannel("Kick");
    channel.stepKey = 36;

    project::Pattern& pattern = project.addPattern("Pattern 1");
    const project::Project original = project;

    REQUIRE(registry.execute(
        std::make_unique<app::ToggleStepCommand>(stepAt(pattern.id, channel.id, 4, 36))));

    const std::vector<project::MidiEvent>* events = project.findPattern(pattern.id)->events(channel.id);
    REQUIRE(events != nullptr);
    REQUIRE(events->size() == 1);
    CHECK((*events)[0].type == project::MidiEventType::note);
    CHECK((*events)[0].tick == step16 * 4);
    CHECK((*events)[0].key == 36);

    // The same note, seen by the other editor. No conversion, no second type.
    app::PianoRollModel roll;
    app::PianoRollModel::Viewport viewport;
    viewport.firstTick    = 0;
    viewport.visibleTicks = ticksPerQuarterNote * 4;
    viewport.lowestKey    = 24;
    viewport.visibleKeys  = 36;
    roll.setViewport(viewport);

    std::vector<app::PianoRollModel::VisibleNote> visible;
    roll.collectVisibleNotes(*events, visible);
    REQUIRE(visible.size() == 1);
    CHECK(visible[0].key == 36);

    // Toggling the same cell again clears it.
    REQUIRE(registry.execute(
        std::make_unique<app::ToggleStepCommand>(stepAt(pattern.id, channel.id, 4, 36))));
    CHECK(project.findPattern(pattern.id)->events(channel.id)->empty());

    while (registry.canUndo())
        REQUIRE(registry.undo());

    CHECK(project == original);
}

TEST_CASE("a note nudged off the grid still belongs to its step")
{
    project::Project project;

    const project::EntityId channel = project.addChannel("Channel 1").id;
    project::Pattern& pattern = project.addPattern("Pattern 1");

    // Placed 10 ticks late, as a Piano Roll edit or a humanise pass would leave
    // it. The rack must keep showing that step as programmed.
    pattern.contentFor(channel).events.push_back(note(step16 * 2 + 10));

    const std::vector<project::MidiEvent>& events = *pattern.events(channel);
    CHECK(app::noteAtStep(events, step16 * 2, step16, 60) == 0);
    CHECK(app::noteAtStep(events, step16 * 3, step16, 60) == app::noStep);
    CHECK(app::noteAtStep(events, step16 * 2, step16, 61) == app::noStep);
}

TEST_CASE("clearing a step restores the note it removed, in place")
{
    project::Project project;
    app::CommandRegistry registry{project};

    const project::EntityId channel = project.addChannel("Channel 1").id;
    project::Pattern& pattern = project.addPattern("Pattern 1");

    auto& events = pattern.contentFor(channel).events;
    events.push_back(note(0));
    events.push_back(note(step16 * 2));
    events.push_back(note(step16 * 4));

    const project::Project original = project;

    REQUIRE(registry.execute(
        std::make_unique<app::ToggleStepCommand>(stepAt(pattern.id, channel, 2))));
    CHECK(project.findPattern(pattern.id)->events(channel)->size() == 2);

    REQUIRE(registry.undo());
    CHECK(project == original);
}

// ── Rack geometry ─────────────────────────────────────────────────────────────

TEST_CASE("rack hit testing finds rows, buttons and steps")
{
    app::ChannelRackModel rack;
    project::Pattern pattern;
    pattern.length = ticksPerQuarterNote * 4;    // 16 steps

    const auto& layout = rack.layout();

    CHECK(rack.stepCount(pattern) == 16);
    CHECK(rack.tickForStep(4) == step16 * 4);

    // A click on the second row's name area.
    const app::ChannelRackModel::Hit name =
        rack.hitTest(4, &pattern, 40.0, rack.rowY(1) + layout.rowHeight / 2.0);
    CHECK(name.row == 1);
    CHECK(name.zone == app::ChannelRackModel::Zone::name);

    const auto mute = rack.muteRect(1);
    const app::ChannelRackModel::Hit muteHit =
        rack.hitTest(4, &pattern, mute.x + 1.0, mute.y + 1.0);
    CHECK(muteHit.zone == app::ChannelRackModel::Zone::mute);

    const auto solo = rack.soloRect(1);
    CHECK(rack.hitTest(4, &pattern, solo.x + 1.0, solo.y + 1.0).zone
          == app::ChannelRackModel::Zone::solo);

    const auto volume = rack.volumeRect(1);
    const app::ChannelRackModel::Hit volumeHit =
        rack.hitTest(4, &pattern, volume.x + volume.width / 2.0, volume.y + 1.0);
    CHECK(volumeHit.zone == app::ChannelRackModel::Zone::volume);

    // A step in the first row.
    const auto cell = rack.stepRect(0, 3);
    const app::ChannelRackModel::Hit stepHit =
        rack.hitTest(4, &pattern, cell.x + 2.0, cell.y + 2.0);
    CHECK(stepHit.zone == app::ChannelRackModel::Zone::step);
    CHECK(stepHit.step == 3);
    CHECK(stepHit.row == 0);

    // Past the end of the pattern there is nothing to toggle.
    const auto beyond = rack.stepRect(0, 16);
    CHECK(rack.hitTest(4, &pattern, beyond.x + 2.0, beyond.y + 2.0).zone
          == app::ChannelRackModel::Zone::none);

    // The gap between rows belongs to neither.
    CHECK(rack.hitTest(4, &pattern, 40.0, layout.rowHeight + 0.5).zone
          == app::ChannelRackModel::Zone::none);

    // Below the last row.
    CHECK(rack.hitTest(4, &pattern, 40.0, rack.rowY(9)).row == app::ChannelRackModel::noRow);
}

TEST_CASE("rack scrolling shifts which steps the grid addresses")
{
    app::ChannelRackModel rack;
    project::Pattern pattern;
    pattern.length = ticksPerQuarterNote * 8;    // 32 steps

    rack.setFirstStep(16);

    const auto cell = rack.stepRect(0, 16);
    CHECK(cell.x == doctest::Approx(rack.layout().headerWidth));

    const app::ChannelRackModel::Hit hit = rack.hitTest(1, &pattern, cell.x + 2.0, cell.y + 2.0);
    CHECK(hit.step == 16);
}

TEST_CASE("a partial step at the end of a pattern is still a step")
{
    app::ChannelRackModel rack;
    project::Pattern pattern;
    pattern.length = ticksPerQuarterNote * 4 + 1;

    CHECK(rack.stepCount(pattern) == 17);
}

// ── Persistence ───────────────────────────────────────────────────────────────

TEST_CASE("a channel's step key survives a save and load")
{
    namespace fs = std::filesystem;

    project::Project project;
    project::Channel& channel = project.addChannel("Kick");
    channel.stepKey = 36;

    project::Pattern& pattern = project.addPattern("Pattern 1");
    pattern.contentFor(channel.id).events.push_back(note(0, 36));

    const fs::path path = fs::temp_directory_path() / "incdaw-stepkey.incdaw";
    std::error_code code;
    fs::remove_all(path, code);

    REQUIRE(project::ProjectFile::save(project, path).succeeded);

    project::Project loaded;
    const auto result = project::ProjectFile::load(loaded, path);
    REQUIRE(result.succeeded);
    CHECK_FALSE(result.migrated);

    REQUIRE(loaded.channels().size() == 1);
    CHECK(loaded.channels().front().stepKey == 36);
    CHECK(loaded == project);

    fs::remove_all(path, code);
}

TEST_CASE("the rack's step ruler pushes the rows down and is not a row itself")
{
    // The band over the grid is where the beats are numbered. It is layout,
    // not content: rows move down by exactly its height, and a click in it must
    // not toggle the step it sits above.
    app::ChannelRackModel rack;

    project::Pattern pattern;
    pattern.length = ticksPerQuarterNote * 4;   // 16 steps

    const double withoutRuler = rack.rowY(0);
    CHECK(withoutRuler == 0.0);

    app::ChannelRackModel::Layout layout = rack.layout();
    layout.rulerHeight = 20.0;
    rack.setLayout(layout);

    CHECK(rack.rowY(0) == doctest::Approx(20.0));
    CHECK(rack.rowY(2) == doctest::Approx(20.0 + 2.0 * rack.rowPitch()));
    CHECK(rack.contentHeight(2) == doctest::Approx(20.0 + 2.0 * rack.rowPitch()
                                                   - layout.rowGap));

    // The ruler band answers nothing.
    for (double y : {0.0, 5.0, 19.0}) {
        const app::ChannelRackModel::Hit hit =
            rack.hitTest(4, &pattern, layout.headerWidth + 5.0, y);
        CHECK(hit.zone == app::ChannelRackModel::Zone::none);
    }

    // The first row now begins where the ruler ends, and its steps still work.
    const auto cell = rack.stepRect(0, 2);
    const app::ChannelRackModel::Hit step =
        rack.hitTest(4, &pattern, cell.x + cell.width / 2.0, cell.y + cell.height / 2.0);

    CHECK(step.row == 0);
    CHECK(step.zone == app::ChannelRackModel::Zone::step);
    CHECK(step.step == 2);

    // The ruler's own cells line up with the pads under them.
    CHECK(rack.rulerStepRect(2).x == doctest::Approx(cell.x));
    CHECK(rack.rulerRect(400.0).height == doctest::Approx(20.0));
}

// ── Pan, and the two-line row that gave it somewhere to live ─────────────────
//
// Channel::pan has been in the model since Phase 4 and applied by the graph
// compiler since Phase 10. It was serialized, compiled and audible, and no
// command could set it — the rack's row had one line and no space left on it.

TEST_CASE("panning a channel round-trips through undo")
{
    project::Project project;
    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::AddChannelCommand>("Kick")));

    const project::Project before = project;
    const project::EntityId id = project.channels().front().id;

    REQUIRE(registry.execute(std::make_unique<app::SetChannelPanCommand>(id, -0.6)));
    CHECK(project.findChannel(id)->pan == doctest::Approx(-0.6));

    REQUIRE(registry.undo());
    CHECK(project == before);
}

TEST_CASE("pan is clamped to the stereo field, and centre is reachable")
{
    project::Project project;
    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::AddChannelCommand>("Kick")));
    const project::EntityId id = project.channels().front().id;

    REQUIRE(registry.execute(std::make_unique<app::SetChannelPanCommand>(id, 4.0)));
    CHECK(project.findChannel(id)->pan == doctest::Approx(1.0));

    REQUIRE(registry.execute(std::make_unique<app::SetChannelPanCommand>(id, -4.0)));
    CHECK(project.findChannel(id)->pan == doctest::Approx(-1.0));

    // Centre is a value, not the absence of one: the double-click that
    // recentres a knob must leave an undo entry like any other edit.
    REQUIRE(registry.execute(std::make_unique<app::SetChannelPanCommand>(id, 0.0)));
    CHECK(project.findChannel(id)->pan == doctest::Approx(0.0));
}

TEST_CASE("a pan command that changes nothing leaves no undo entry")
{
    project::Project project;
    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::AddChannelCommand>("Kick")));
    const project::EntityId id = project.channels().front().id;

    const std::size_t depth = registry.undoDepth();

    // A fresh channel is already centred.
    CHECK_FALSE(registry.execute(std::make_unique<app::SetChannelPanCommand>(id, 0.0)));
    CHECK(registry.undoDepth() == depth);
}

TEST_CASE("dragging a pan knob is one undo, and returns to where the drag began")
{
    project::Project project;
    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::AddChannelCommand>("Kick")));
    const project::EntityId id = project.channels().front().id;

    const std::size_t depth = registry.undoDepth();

    // Every point of the drag, the first included: merging cannot tell one
    // gesture from the next, so the entry the drag opens is the whole drag.
    for (const double pan : {-0.4, -0.2, 0.1, 0.35})
        REQUIRE(registry.executeMerging(std::make_unique<app::SetChannelPanCommand>(id, pan)));

    CHECK(registry.undoDepth() == depth + 1);
    CHECK(project.findChannel(id)->pan == doctest::Approx(0.35));

    // One undo returns to before the drag, not to its second-to-last position.
    REQUIRE(registry.undo());
    CHECK(project.findChannel(id)->pan == doctest::Approx(0.0));
}

TEST_CASE("a pan drag is vertical, travels further than the knob, and pins at the ends")
{
    using app::ChannelRackModel;

    // Up is right: a knob turning clockwise under an upward drag.
    CHECK(ChannelRackModel::panForDrag(0.0, 100.0, 40.0) > 0.0);
    CHECK(ChannelRackModel::panForDrag(0.0, 100.0, 160.0) < 0.0);

    // The travel is the whole bipolar sweep, so from centre half of it reaches
    // an end and a quarter of it reaches halfway.
    CHECK(ChannelRackModel::panForDrag(0.0, 100.0, 100.0 - ChannelRackModel::knobDragTravel / 2.0)
          == doctest::Approx(1.0));
    CHECK(ChannelRackModel::panForDrag(0.0, 100.0, 100.0 - ChannelRackModel::knobDragTravel / 4.0)
          == doctest::Approx(0.5));

    // Past the ends it pins rather than wrapping, and it starts from where the
    // drag began rather than from centre.
    CHECK(ChannelRackModel::panForDrag(0.0, 100.0, -900.0) == doctest::Approx(1.0));
    CHECK(ChannelRackModel::panForDrag(0.0, 100.0, 900.0) == doctest::Approx(-1.0));
    CHECK(ChannelRackModel::panForDrag(-1.0, 100.0, 100.0) == doctest::Approx(-1.0));
}

TEST_CASE("a row reads left to right and every control stays inside the header")
{
    app::ChannelRackModel model;

    const app::ChannelRackModel::Layout layout = model.layout();
    const auto row = model.rowRect(0);

    const auto lamp   = model.muteRect(0);
    const auto solo   = model.soloRect(0);
    const auto name   = model.nameRect(0);
    const auto volume = model.volumeRect(0);
    const auto pan    = model.panRect(0);

    // Lamp, switch, button, volume, pan — the order a step sequencer's row has
    // had since the hardware ones.
    CHECK(lamp.x + lamp.width <= solo.x);
    CHECK(solo.x + solo.width <= name.x);
    CHECK(name.x + name.width <= volume.x);
    CHECK(volume.x + volume.width <= pan.x);

    // Nothing escapes the header, and everything is vertically inside the row.
    for (const auto& rect : {lamp, solo, name, volume, pan}) {
        CHECK(rect.x >= 0.0);
        CHECK(rect.x + rect.width <= layout.headerWidth + 0.001);
        CHECK(rect.y >= row.y);
        CHECK(rect.y + rect.height <= row.y + row.height + 0.001);
    }

    // The lamp IS the colour swatch: one control carries identity and state,
    // and a separate swatch would say the same thing twice.
    const auto swatch = model.swatchRect(0);
    CHECK(swatch.x == doctest::Approx(lamp.x));
    CHECK(swatch.width == doctest::Approx(lamp.width));

    // Both knobs are square, so they read as knobs and not as short faders.
    CHECK(pan.width == doctest::Approx(pan.height));
    CHECK(volume.width == doctest::Approx(volume.height));

    // The button is the largest target in the row: it is the one that opens
    // the channel's instrument.
    CHECK(name.width > pan.width + volume.width);
}

TEST_CASE("each knob is its own hit zone, and the button takes the space between")
{
    app::ChannelRackModel model;

    const auto pan    = model.panRect(0);
    const auto volume = model.volumeRect(0);

    const auto onPan = model.hitTest(1, nullptr, pan.x + pan.width / 2.0,
                                     pan.y + pan.height / 2.0);
    CHECK(onPan.zone == app::ChannelRackModel::Zone::pan);
    CHECK(onPan.row == 0);

    const auto onVolume = model.hitTest(1, nullptr, volume.x + volume.width / 2.0,
                                        volume.y + volume.height / 2.0);
    CHECK(onVolume.zone == app::ChannelRackModel::Zone::volume);

    const auto onName = model.hitTest(1, nullptr, model.contentLeft() + 2.0,
                                      model.rowRect(0).y + model.layout().rowHeight / 2.0);
    CHECK(onName.zone == app::ChannelRackModel::Zone::name);

    // The lamp is the mute switch.
    const auto lamp = model.muteRect(0);
    CHECK(model.hitTest(1, nullptr, lamp.x + lamp.width / 2.0,
                        lamp.y + lamp.height / 2.0).zone
          == app::ChannelRackModel::Zone::mute);
}

TEST_CASE("steps are grouped in fours, and the gap between groups is not a step")
{
    app::ChannelRackModel model;

    project::Project project;
    project::Pattern pattern;
    pattern.length = ticksPerQuarterNote * 8;    // 32 steps
    project.patterns().push_back(pattern);

    const app::ChannelRackModel::Layout layout = model.layout();

    // Within a group the cells are at the plain pitch; crossing a boundary
    // costs one extra gap.
    const double pitch = layout.stepWidth + layout.stepGap;

    CHECK(model.stepOffset(1) - model.stepOffset(0) == doctest::Approx(pitch));
    CHECK(model.stepOffset(4) - model.stepOffset(3)
          == doctest::Approx(pitch + layout.stepGroupGap));
    CHECK(model.stepOffset(8) - model.stepOffset(7)
          == doctest::Approx(pitch + layout.stepGroupGap));

    // The hit test agrees with the geometry at every step, including the ones
    // pushed out of line by the accumulated gaps.
    for (int step = 0; step < 32; ++step) {
        const auto cell = model.stepRect(0, step);
        const auto hit  = model.hitTest(1, &project.patterns().front(),
                                        cell.x + cell.width / 2.0, cell.y + 2.0);

        CHECK(hit.zone == app::ChannelRackModel::Zone::step);
        CHECK(hit.step == step);
    }

    // Landing in the group gap is landing on nothing, like the gap between two
    // cells inside a group.
    const auto lastOfGroup = model.stepRect(0, 3);
    const auto gapHit = model.hitTest(1, &project.patterns().front(),
                                      lastOfGroup.x + lastOfGroup.width + 2.0,
                                      lastOfGroup.y + 2.0);
    CHECK(gapHit.zone == app::ChannelRackModel::Zone::none);
    CHECK(gapHit.row == 0);
}

TEST_CASE("scrolling does not move where the bars appear to start")
{
    app::ChannelRackModel model;
    const app::ChannelRackModel::Layout layout = model.layout();
    const double pitch = layout.stepWidth + layout.stepGap;

    // Scrolled to a group boundary, the grid looks exactly as it did at zero:
    // the grouping belongs to the bar, not to the scroll position.
    model.setFirstStep(4);
    CHECK(model.stepOffset(4) == doctest::Approx(0.0));
    CHECK(model.stepOffset(5) - model.stepOffset(4) == doctest::Approx(pitch));
    CHECK(model.stepOffset(8) - model.stepOffset(7)
          == doctest::Approx(pitch + layout.stepGroupGap));
}
