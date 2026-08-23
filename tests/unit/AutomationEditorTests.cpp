// TRACK B (B9) — the automation editor.
//
// Lanes rendered in the playlist and recorded from a write pass, and there was
// no surface for editing them. The surface is a model of the same shape as
// PlaylistModel: geometry, hit testing, selection and edits, all arithmetic and
// all testable without a window.
//
// Two properties are worth more than the rest:
//
//   * every edit is a pure function from one point vector to the next, so all
//     of them go through the one SetAutomationPointsCommand the subsystem
//     already had — draw, erase, drag, curve, scale and paste are one undo
//     path, not six;
//   * the curve the editor draws is evaluated by engine::AutomationSequence,
//     the class the audio thread reads, so the shape on screen cannot drift
//     from the shape being played.

#include "doctest.h"

#include "app/AutomationEditorModel.h"
#include "app/CommandRegistry.h"
#include "app/commands/AutomationCommands.h"
#include "engine/automation/AutomationSequence.h"
#include "project/Model.h"

#include <memory>
#include <vector>

using namespace incdaw;
using app::AutomationEditorModel;
using engine::Tick;
using engine::ticksPerQuarterNote;
using project::AutomationCurve;
using project::AutomationPoint;

namespace {

AutomationPoint at(Tick tick, double value,
                   AutomationCurve curve = AutomationCurve::linear)
{
    AutomationPoint point;
    point.tick  = tick;
    point.value = value;
    point.curve = curve;
    return point;
}

/// A model over eight bars, 800 points wide and 200 tall with a 20-point ruler.
AutomationEditorModel makeEditor()
{
    AutomationEditorModel editor;

    AutomationEditorModel::Viewport viewport;
    viewport.firstTick    = 0;
    viewport.visibleTicks = ticksPerQuarterNote * 4 * 8;
    viewport.width        = 800.0;
    viewport.height       = 200.0;
    viewport.rulerHeight  = 20.0;
    editor.setViewport(viewport);

    return editor;
}

/// A ramp from silence to full over two bars, with a step in the middle.
std::vector<AutomationPoint> ramp()
{
    return {
        at(0, 0.0),
        at(ticksPerQuarterNote * 4, 0.5, AutomationCurve::hold),
        at(ticksPerQuarterNote * 8, 1.0),
    };
}

} // namespace

// ── Geometry ─────────────────────────────────────────────────────────────────

TEST_CASE("value one is at the top of the curve band, zero at the bottom")
{
    const AutomationEditorModel editor = makeEditor();

    // The curve band is what is left below the 20-point ruler: 180 tall.
    CHECK(editor.valueToY(1.0) == doctest::Approx(20.0));    // just under the ruler
    CHECK(editor.valueToY(0.0) == doctest::Approx(200.0));
    CHECK(editor.valueToY(0.5) == doctest::Approx(110.0));

    // And back again, for every value the mouse can name.
    for (double value = 0.0; value <= 1.0; value += 0.1)
        CHECK(editor.yToValue(editor.valueToY(value)) == doctest::Approx(value));

    // Off the ends of the band clamps rather than reporting a value no fader
    // could hold.
    CHECK(editor.yToValue(-500.0) == doctest::Approx(1.0));
    CHECK(editor.yToValue(5000.0) == doctest::Approx(0.0));
}

TEST_CASE("a point's handle is a target the pointer can actually hit")
{
    const AutomationEditorModel editor = makeEditor();
    const auto points = ramp();

    const auto rect = editor.pointRect(points[1]);
    CHECK(rect.width == doctest::Approx(AutomationEditorModel::handleRadius * 2.0));

    const std::size_t hit = editor.pointAt(points, rect.x + 1.0, rect.y + 1.0);
    CHECK(hit == 1);

    CHECK(editor.pointAt(points, rect.x + 200.0, rect.y) == AutomationEditorModel::noPoint);
}

TEST_CASE("a click between two points names the segment that starts before it")
{
    const AutomationEditorModel editor = makeEditor();
    const auto points = ramp();

    CHECK(editor.segmentAt(points, editor.tickToX(ticksPerQuarterNote * 2)) == 0);
    CHECK(editor.segmentAt(points, editor.tickToX(ticksPerQuarterNote * 6)) == 1);

    // Before the first point and after the last there is no segment: the
    // envelope holds, and holding is not a segment anyone can shape.
    CHECK(editor.segmentAt(points, editor.tickToX(ticksPerQuarterNote * 20))
          == AutomationEditorModel::noPoint);
}

TEST_CASE("box selection picks the points inside the rectangle")
{
    const AutomationEditorModel editor = makeEditor();
    const auto points = ramp();

    std::vector<std::size_t> boxed;

    // A band across the lower half catches the two points below 0.5 and misses
    // the one at the top.
    editor.pointsInRectangle(points, -10.0, editor.valueToY(0.6),
                             editor.tickToX(ticksPerQuarterNote * 12) + 10.0,
                             editor.valueToY(0.0) - editor.valueToY(0.6) + 10.0, boxed);

    REQUIRE(boxed.size() == 2);
    CHECK(boxed[0] == 0);
    CHECK(boxed[1] == 1);
}

// ── The curve is the engine's ────────────────────────────────────────────────

TEST_CASE("the drawn curve is the played curve")
{
    const auto points = ramp();

    // The reference is the engine's own evaluator, built here by hand from the
    // same numbers: if the editor grew a second interpolation, this diverges.
    engine::AutomationSequence reference;
    reference.setPoints({
        {0, 0.0f, engine::AutomationShape::linear, 0.0f},
        {ticksPerQuarterNote * 4, 0.5f, engine::AutomationShape::hold, 0.0f},
        {ticksPerQuarterNote * 8, 1.0f, engine::AutomationShape::linear, 0.0f},
    });

    for (Tick tick = 0; tick <= ticksPerQuarterNote * 10; tick += 37)
        CHECK(AutomationEditorModel::valueAt(points, tick)
              == doctest::Approx(static_cast<double>(reference.valueAt(tick))));

    // The hold really holds: the whole second segment sits at 0.5.
    CHECK(AutomationEditorModel::valueAt(points, ticksPerQuarterNote * 7)
          == doctest::Approx(0.5));

    CHECK(AutomationEditorModel::valueAt({}, 0) == doctest::Approx(0.0));
}

TEST_CASE("the curve is sampled across the viewport for drawing")
{
    const AutomationEditorModel editor = makeEditor();

    std::vector<double> sweep;
    editor.collectCurve(ramp(), sweep, 8.0);

    REQUIRE(sweep.size() > 90);
    CHECK(sweep.front() == doctest::Approx(0.0));

    for (const double value : sweep)
        CHECK(value >= 0.0);

    std::vector<double> empty;
    editor.collectCurve({}, empty);
    CHECK(empty.empty());
}

// ── Drawing and erasing ──────────────────────────────────────────────────────

TEST_CASE("a point is drawn in, and a second on the same tick replaces it")
{
    auto points = AutomationEditorModel::withPointAdded(ramp(), ticksPerQuarterNote * 2, 0.25);

    REQUIRE(points.size() == 4);
    CHECK(points[1].tick == ticksPerQuarterNote * 2);
    CHECK(points[1].value == doctest::Approx(0.25));

    // Drawing over it replaces rather than stacking: two points on one tick is
    // a segment of zero length.
    points = AutomationEditorModel::withPointAdded(points, ticksPerQuarterNote * 2, 0.75);
    REQUIRE(points.size() == 4);
    CHECK(points[1].value == doctest::Approx(0.75));

    // Values and ticks are clamped to what a lane can hold.
    points = AutomationEditorModel::withPointAdded(points, -500, 9.0);
    CHECK(points.front().tick == 0);
    CHECK(points.front().value == doctest::Approx(1.0));
}

TEST_CASE("erasing takes the named points and nothing else")
{
    const auto points = AutomationEditorModel::withPointsRemoved(ramp(), {0, 2});

    REQUIRE(points.size() == 1);
    CHECK(points[0].tick == ticksPerQuarterNote * 4);

    // Indices past the end are ignored rather than being an error: a selection
    // can outlive the edit that shortened the lane.
    CHECK(AutomationEditorModel::withPointsRemoved(ramp(), {99}).size() == 3);
}

// ── Dragging ─────────────────────────────────────────────────────────────────

TEST_CASE("dragging points moves them in time and value, and undoes through the command")
{
    project::Project project;
    project::AutomationLane& lane =
        project.addAutomationLane(project::EntityId{1}, "gain");
    lane.points = ramp();

    const project::EntityId laneId = lane.id;

    app::CommandRegistry registry{project};

    const auto moved = AutomationEditorModel::withPointsMoved(
        lane.points, {1}, ticksPerQuarterNote, 0.25);

    REQUIRE(registry.execute(std::make_unique<app::SetAutomationPointsCommand>(
        laneId, moved, "Move Automation")));

    const project::AutomationLane* after = nullptr;
    for (const project::AutomationLane& entry : project.automation())
        if (entry.id == laneId)
            after = &entry;

    REQUIRE(after != nullptr);
    CHECK(after->points[1].tick == ticksPerQuarterNote * 5);
    CHECK(after->points[1].value == doctest::Approx(0.75));

    REQUIRE(registry.undo());

    for (const project::AutomationLane& entry : project.automation())
        if (entry.id == laneId)
            CHECK(entry.points[1].tick == ticksPerQuarterNote * 4);
}

TEST_CASE("a dragged group clamps as one, so it keeps its shape")
{
    const auto points = AutomationEditorModel::withPointsMoved(
        ramp(), {0, 1, 2}, -ticksPerQuarterNote * 100, -5.0);

    // The point already at tick zero and value zero pins the set: everything
    // holds its distance from it.
    CHECK(points[0].tick == 0);
    CHECK(points[1].tick == ticksPerQuarterNote * 4);
    CHECK(points[2].tick == ticksPerQuarterNote * 8);

    CHECK(points[0].value == doctest::Approx(0.0));
    CHECK(points[1].value == doctest::Approx(0.5));
}

TEST_CASE("a drag that lands two points on one tick keeps one of them")
{
    const auto points = AutomationEditorModel::withPointsMoved(
        ramp(), {1}, ticksPerQuarterNote * 4, 0.0);

    // Point 1 moved onto point 2's tick; a zero-length segment cannot exist.
    CHECK(points.size() == 2);
}

// ── Curve, tension, scaling ──────────────────────────────────────────────────

TEST_CASE("curve and tension are set per segment, on the point that starts it")
{
    auto points = AutomationEditorModel::withCurve(ramp(), {0}, AutomationCurve::smooth);
    CHECK(points[0].curve == AutomationCurve::smooth);
    CHECK(points[1].curve == AutomationCurve::hold);   // untouched

    points = AutomationEditorModel::withTension(points, {0}, 0.6);
    CHECK(points[0].tension == doctest::Approx(0.6));

    points = AutomationEditorModel::withTension(points, {0}, 4.0);
    CHECK(points[0].tension == doctest::Approx(1.0));

    // And the shape actually changes what is played.
    CHECK(AutomationEditorModel::valueAt(points, ticksPerQuarterNote * 2)
          != doctest::Approx(AutomationEditorModel::valueAt(ramp(), ticksPerQuarterNote * 2)));
}

TEST_CASE("a selection scales in time about an anchor")
{
    const auto points = AutomationEditorModel::withTimeScaled(
        ramp(), {0, 1, 2}, 0.5, 0);

    CHECK(points[0].tick == 0);
    CHECK(points[1].tick == ticksPerQuarterNote * 2);
    CHECK(points[2].tick == ticksPerQuarterNote * 4);

    // Anchored elsewhere, the anchor is what stays put.
    const auto about = AutomationEditorModel::withTimeScaled(
        ramp(), {0, 1, 2}, 2.0, ticksPerQuarterNote * 4);

    CHECK(about[1].tick == ticksPerQuarterNote * 4);
    CHECK(about[0].tick == 0);                          // -4 doubled to -8, clamped
    CHECK(about[2].tick == ticksPerQuarterNote * 12);
}

TEST_CASE("a selection scales in value about an anchor, and cannot leave the lane")
{
    const auto points = AutomationEditorModel::withValueScaled(ramp(), {0, 1, 2}, 0.5, 0.5);

    CHECK(points[0].value == doctest::Approx(0.25));
    CHECK(points[1].value == doctest::Approx(0.5));
    CHECK(points[2].value == doctest::Approx(0.75));

    const auto blown = AutomationEditorModel::withValueScaled(ramp(), {0, 1, 2}, 10.0, 0.5);
    CHECK(blown[0].value == doctest::Approx(0.0));
    CHECK(blown[2].value == doctest::Approx(1.0));
}

// ── Copy and paste, between lanes ────────────────────────────────────────────

TEST_CASE("a copied selection is rebased, so a paste is a translation")
{
    const auto clipboard = AutomationEditorModel::copyOf(ramp(), {1, 2});

    REQUIRE(clipboard.size() == 2);
    CHECK(clipboard[0].tick == 0);
    CHECK(clipboard[1].tick == ticksPerQuarterNote * 4);
    CHECK(clipboard[0].curve == AutomationCurve::hold);   // the shape travels with it
}

TEST_CASE("a lane's points paste into a different lane")
{
    const auto clipboard = AutomationEditorModel::copyOf(ramp(), {0, 1});

    // An empty lane for a different parameter entirely: a lane is a list of
    // normalised points and nothing else, which is what makes this legal.
    std::vector<AutomationPoint> other;
    other = AutomationEditorModel::withPasted(other, clipboard, ticksPerQuarterNote * 16);

    REQUIRE(other.size() == 2);
    CHECK(other[0].tick == ticksPerQuarterNote * 16);
    CHECK(other[1].tick == ticksPerQuarterNote * 20);
    CHECK(other[1].value == doctest::Approx(0.5));

    const auto landed = AutomationEditorModel::pastedIndices(other, clipboard,
                                                             ticksPerQuarterNote * 16);
    REQUIRE(landed.size() == 2);
    CHECK(landed[0] == 0);
    CHECK(landed[1] == 1);
}

TEST_CASE("a paste replaces whatever it lands on")
{
    const auto clipboard = AutomationEditorModel::copyOf(ramp(), {0});

    const auto pasted = AutomationEditorModel::withPasted(ramp(), clipboard,
                                                          ticksPerQuarterNote * 4);

    // One point still at that tick, and it is the pasted one.
    std::size_t count = 0;
    for (const AutomationPoint& point : pasted)
        if (point.tick == ticksPerQuarterNote * 4)
            ++count;

    CHECK(count == 1);
    CHECK(pasted.size() == 3);
}

// ── Selection bookkeeping ────────────────────────────────────────────────────

TEST_CASE("the selection is a set, and is pruned when an edit shortens the lane")
{
    AutomationEditorModel editor = makeEditor();

    editor.setSelection({2, 0, 2});
    CHECK(editor.selection().size() == 2);
    CHECK(editor.isSelected(0));
    CHECK(editor.isSelected(2));

    editor.toggleSelection(0);
    CHECK_FALSE(editor.isSelected(0));

    editor.addToSelection(1);
    CHECK(editor.selection().size() == 2);

    editor.pruneSelection(2);
    CHECK(editor.selection().size() == 1);
    CHECK(editor.isSelected(1));

    editor.clearSelection();
    CHECK(editor.selection().empty());
}

TEST_CASE("the grid rounds to the nearest division, and off means free")
{
    AutomationEditorModel editor = makeEditor();

    editor.setSnap(ticksPerQuarterNote);
    CHECK(editor.snapTick(ticksPerQuarterNote / 2 - 1) == 0);
    CHECK(editor.snapTick(ticksPerQuarterNote / 2) == ticksPerQuarterNote);
    CHECK(editor.snapTick(ticksPerQuarterNote * 3) == ticksPerQuarterNote * 3);

    editor.setSnap(0);
    CHECK(editor.snapTick(7) == 7);
}

// ── TRACK B (B10) — the automation clip workflow ─────────────────────────────
//
// "Automate this parameter" was reachable only by recording a pass in write
// mode: there was no way to ask for a lane. These are the verbs that make one
// on purpose, and the one that unshares a lane two clips are riding.

TEST_CASE("creating an automation clip makes the lane, the track and the clip together")
{
    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    const project::EntityId master = project.masterMixerNode();

    app::CommandRegistry registry{project};

    auto command = std::make_unique<app::CreateAutomationClipCommand>(
        master, "volume", ticksPerQuarterNote * 8, ticksPerQuarterNote * 16, 0.75);
    app::CreateAutomationClipCommand* raw = command.get();

    REQUIRE(registry.execute(std::move(command)));

    REQUIRE(project.automation().size() == 1);
    const project::AutomationLane& lane = project.automation()[0];
    CHECK(lane.targetEntity == master);
    CHECK(lane.parameterKey == "volume");

    // Seeded flat at the control's current value: an empty lane would read as
    // zero the moment it played.
    REQUIRE(lane.points.size() == 2);
    CHECK(lane.points[0].value == doctest::Approx(0.75));
    CHECK(lane.points[1].value == doctest::Approx(0.75));
    CHECK(lane.points[0].tick == ticksPerQuarterNote * 8);
    CHECK(lane.points[1].tick == ticksPerQuarterNote * 24);

    // An automation track was made to hold it.
    REQUIRE(project.tracks().size() == 1);
    CHECK(project.tracks()[0].type == project::TrackType::automation);

    const project::Clip* clip = project.findClip(raw->clipId());
    REQUIRE(clip != nullptr);
    CHECK(clip->type == project::ClipType::automation);
    CHECK(clip->source == lane.id);
    CHECK(clip->startTick == ticksPerQuarterNote * 8);

    // One undo takes all three back, and redo restores the same ids.
    REQUIRE(registry.undo());
    CHECK(project.automation().empty());
    CHECK(project.clips().empty());
    CHECK(project.tracks().empty());

    REQUIRE(registry.redo());
    REQUIRE(project.automation().size() == 1);
    CHECK(project.automation()[0].id == lane.id);
    CHECK(project.findClip(raw->clipId()) != nullptr);
}

TEST_CASE("a second clip for the same parameter reuses the lane rather than forking it")
{
    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    const project::EntityId master = project.masterMixerNode();
    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::CreateAutomationClipCommand>(
        master, "pan", 0, ticksPerQuarterNote * 4)));
    REQUIRE(registry.execute(std::make_unique<app::CreateAutomationClipCommand>(
        master, "pan", ticksPerQuarterNote * 16, ticksPerQuarterNote * 4)));

    CHECK(project.automation().size() == 1);   // one ride, placed twice
    REQUIRE(project.clips().size() == 2);
    CHECK(project.clips()[0].source == project.clips()[1].source);
    CHECK(project.tracks().size() == 1);       // and one track, not two
}

TEST_CASE("making a shared lane unique gives the clip a ride of its own")
{
    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    const project::EntityId master = project.masterMixerNode();
    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::CreateAutomationClipCommand>(
        master, "volume", 0, ticksPerQuarterNote * 4)));
    REQUIRE(registry.execute(std::make_unique<app::CreateAutomationClipCommand>(
        master, "volume", ticksPerQuarterNote * 8, ticksPerQuarterNote * 4)));

    const project::EntityId shared = project.clips()[0].source;
    const project::EntityId second = project.clips()[1].id;

    REQUIRE(registry.execute(std::make_unique<app::MakeAutomationClipUniqueCommand>(second)));

    REQUIRE(project.automation().size() == 2);
    CHECK(project.findClip(second)->source != shared);
    CHECK(project.clips()[0].source == shared);

    // The copy starts as a copy: same points, its own identity.
    const project::AutomationLane& copy = project.automation()[1];
    CHECK(copy.points.size() == 2);
    CHECK(copy.parameterKey == "volume");

    // Editing the copy leaves the original alone — which is the whole point.
    REQUIRE(registry.execute(std::make_unique<app::SetAutomationPointsCommand>(
        copy.id, std::vector<AutomationPoint>{at(0, 1.0)})));

    CHECK(project.automation()[0].points.size() == 2);

    REQUIRE(registry.undo());
    REQUIRE(registry.undo());
    CHECK(project.automation().size() == 1);
    CHECK(project.findClip(second)->source == shared);
}

TEST_CASE("a clip that is already alone on its lane is already unique")
{
    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    app::CommandRegistry registry{project};

    auto command = std::make_unique<app::CreateAutomationClipCommand>(
        project.masterMixerNode(), "volume", 0, ticksPerQuarterNote * 4);
    app::CreateAutomationClipCommand* raw = command.get();
    REQUIRE(registry.execute(std::move(command)));

    CHECK_FALSE(registry.execute(
        std::make_unique<app::MakeAutomationClipUniqueCommand>(raw->clipId())));
    CHECK(project.automation().size() == 1);
}

TEST_CASE("clearing a lane is the same command with an empty vector")
{
    project::Project project;
    project::AutomationLane& lane = project.addAutomationLane(project::EntityId{1}, "gain");
    lane.points = ramp();

    const project::EntityId laneId = lane.id;
    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::SetAutomationPointsCommand>(
        laneId, std::vector<AutomationPoint>{}, "Clear Automation")));

    CHECK(project.automation()[0].points.empty());

    REQUIRE(registry.undo());
    CHECK(project.automation()[0].points.size() == 3);
}
