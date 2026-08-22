#include "doctest.h"

#include "app/PianoRollModel.h"
#include "engine/core/RealtimeGuard.h"

#include <chrono>

using namespace incdaw;
using namespace incdaw::app;
using incdaw::engine::ticksPerQuarterNote;

namespace {

project::MidiEvent note(Tick tick, int key, Tick duration = 240)
{
    project::MidiEvent event;
    event.type     = project::MidiEventType::note;
    event.tick     = tick;
    event.key      = key;
    event.duration = duration;
    return event;
}

/// One bar wide, three octaves tall, at a comfortable editing zoom.
PianoRollModel makeModel()
{
    PianoRollModel model;

    PianoRollModel::Viewport viewport;
    viewport.firstTick    = 0;
    viewport.visibleTicks = ticksPerQuarterNote * 4;
    viewport.lowestKey    = 48;
    viewport.visibleKeys  = 36;
    viewport.width        = 1200.0;
    viewport.height       = 720.0;

    model.setViewport(viewport);
    return model;
}

} // namespace

// ── Geometry ──────────────────────────────────────────────────────────────────

TEST_CASE("ticks and x coordinates round-trip")
{
    const PianoRollModel model = makeModel();

    for (Tick tick = 0; tick < ticksPerQuarterNote * 4; tick += 37)
        CHECK(model.xToTick(model.tickToX(tick)) == tick);
}

TEST_CASE("keys and y coordinates round-trip, with higher pitches drawn higher")
{
    const PianoRollModel model = makeModel();

    for (int key = 48; key < 84; ++key)
        CHECK(model.yToKey(model.keyToY(key) + 1.0) == key);

    // A higher note must have a smaller y.
    CHECK(model.keyToY(72) < model.keyToY(60));
}

TEST_CASE("zoom is reflected in points per tick and key height")
{
    PianoRollModel model = makeModel();
    const double before = model.pointsPerTick();

    auto viewport = model.viewport();
    viewport.visibleTicks *= 2;      // zoomed out: same width shows twice the music
    model.setViewport(viewport);

    CHECK(model.pointsPerTick() == doctest::Approx(before / 2.0));
    CHECK(model.keyHeight() == doctest::Approx(720.0 / 36.0));
}

TEST_CASE("a degenerate viewport yields zero scale rather than dividing by zero")
{
    PianoRollModel model;
    PianoRollModel::Viewport viewport;
    viewport.visibleTicks = 0;
    viewport.visibleKeys  = 0;
    model.setViewport(viewport);

    CHECK(model.pointsPerTick() == doctest::Approx(0.0));
    CHECK(model.keyHeight() == doctest::Approx(0.0));

    std::vector<project::MidiEvent> notes;
    notes.push_back(note(0, 60));

    std::vector<PianoRollModel::VisibleNote> visible;
    model.collectVisibleNotes(notes, visible);
    CHECK(visible.empty());
}

// ── Culling ───────────────────────────────────────────────────────────────────

TEST_CASE("only notes intersecting the viewport are collected")
{
    const PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    notes.push_back(note(0, 60));                              // visible
    notes.push_back(note(ticksPerQuarterNote * 2, 72));        // visible
    notes.push_back(note(ticksPerQuarterNote * 8, 60));        // right of view
    notes.push_back(note(0, 20));                              // below view
    notes.push_back(note(0, 100));                             // above view

    std::vector<PianoRollModel::VisibleNote> visible;
    model.collectVisibleNotes(notes, visible);

    REQUIRE(visible.size() == 2);
    CHECK(visible[0].index == 0);
    CHECK(visible[1].index == 1);
}

TEST_CASE("a long note starting before the viewport is still drawn")
{
    // Culling on the start position alone would make long notes vanish as you
    // scroll into them.
    PianoRollModel model = makeModel();

    auto viewport = model.viewport();
    viewport.firstTick = ticksPerQuarterNote * 4;
    model.setViewport(viewport);

    std::vector<project::MidiEvent> notes;
    notes.push_back(note(0, 60, ticksPerQuarterNote * 8));   // spans the viewport

    std::vector<PianoRollModel::VisibleNote> visible;
    model.collectVisibleNotes(notes, visible);

    REQUIRE(visible.size() == 1);
    CHECK(visible[0].x < 0.0);   // starts off the left edge
}

TEST_CASE("non-note events are not drawn as notes")
{
    const PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    project::MidiEvent cc;
    cc.type = project::MidiEventType::controlChange;
    cc.tick = 0;
    notes.push_back(cc);

    std::vector<PianoRollModel::VisibleNote> visible;
    model.collectVisibleNotes(notes, visible);
    CHECK(visible.empty());
}

TEST_CASE("selection state is carried into the visible list")
{
    PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    notes.push_back(note(0, 60));
    notes.push_back(note(480, 62));

    model.setSelection({1});

    std::vector<PianoRollModel::VisibleNote> visible;
    model.collectVisibleNotes(notes, visible);

    REQUIRE(visible.size() == 2);
    CHECK_FALSE(visible[0].selected);
    CHECK(visible[1].selected);
}

// ── Hit testing ───────────────────────────────────────────────────────────────

TEST_CASE("clicking inside a note selects it, and empty space selects nothing")
{
    const PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    notes.push_back(note(ticksPerQuarterNote, 60, 480));

    const double x = model.tickToX(ticksPerQuarterNote + 100);
    const double y = model.keyToY(60) + 2.0;

    CHECK(model.noteAtPoint(notes, x, y) == 0);

    CHECK(model.noteAtPoint(notes, x, model.keyToY(61) + 2.0) == PianoRollModel::noNote);
    CHECK(model.noteAtPoint(notes, model.tickToX(0) + 1.0, y) == PianoRollModel::noNote);
}

TEST_CASE("where notes overlap, the one drawn on top is picked")
{
    const PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    notes.push_back(note(0, 60, 960));
    notes.push_back(note(0, 60, 960));   // same place, drawn later

    const double x = model.tickToX(100);
    const double y = model.keyToY(60) + 2.0;

    CHECK(model.noteAtPoint(notes, x, y) == 1);
}

TEST_CASE("the right edge of a note is a resize handle, the middle is not")
{
    const PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    notes.push_back(note(0, 60, ticksPerQuarterNote));

    const double y     = model.keyToY(60) + 2.0;
    const double right = model.tickToX(ticksPerQuarterNote);

    CHECK(model.isOverResizeHandle(notes, 0, right - 2.0, y));
    CHECK_FALSE(model.isOverResizeHandle(notes, 0, model.tickToX(100), y));
    CHECK_FALSE(model.isOverResizeHandle(notes, 0, right - 2.0, model.keyToY(61) + 2.0));
}

TEST_CASE("a very short note still leaves room to grab and move it")
{
    // If the handle covered the whole note there would be no way to drag it.
    const PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    notes.push_back(note(0, 60, 4));

    const double y    = model.keyToY(60) + 2.0;
    const double left = model.tickToX(0);

    CHECK_FALSE(model.isOverResizeHandle(notes, 0, left + 0.1, y));
}

// ── Box selection ─────────────────────────────────────────────────────────────

TEST_CASE("box selection catches notes that merely intersect the rectangle")
{
    const PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    notes.push_back(note(0, 60, ticksPerQuarterNote * 3));   // long, extends past the box
    notes.push_back(note(0, 72, 120));                        // outside vertically

    std::vector<std::size_t> hits;
    model.notesInRectangle(notes, 0.0, model.keyToY(60), 50.0, model.keyHeight(), hits);

    REQUIRE(hits.size() == 1);
    CHECK(hits[0] == 0);
}

TEST_CASE("a rectangle dragged up and to the left works the same as one dragged down and right")
{
    const PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    notes.push_back(note(ticksPerQuarterNote, 60));

    const double x = model.tickToX(ticksPerQuarterNote) + 5.0;
    const double y = model.keyToY(60) + 5.0;

    std::vector<std::size_t> forward;
    std::vector<std::size_t> backward;

    model.notesInRectangle(notes, x - 40.0, y - 30.0, 80.0, 60.0, forward);
    model.notesInRectangle(notes, x + 40.0, y + 30.0, -80.0, -60.0, backward);

    CHECK(forward == backward);
    CHECK(forward.size() == 1);
}

// ── Snap ──────────────────────────────────────────────────────────────────────

TEST_CASE("snapping rounds to the nearest grid line, and zero means free placement")
{
    PianoRollModel model = makeModel();

    model.setSnap(0);
    CHECK(model.snapTick(137) == 137);

    model.setSnap(ticksPerQuarterNote / 4);   // sixteenths = 240
    CHECK(model.snapTick(0) == 0);
    CHECK(model.snapTick(100) == 0);
    CHECK(model.snapTick(140) == 240);
    CHECK(model.snapTick(240) == 240);
    CHECK(model.snapTick(-100) == 0);
    CHECK(model.snapTick(-140) == -240);
}

// ── Selection bookkeeping ─────────────────────────────────────────────────────

TEST_CASE("selection is deduplicated and order-independent")
{
    PianoRollModel model;
    model.setSelection({5, 1, 5, 3, 1});

    CHECK(model.selection().size() == 3);
    CHECK(model.isSelected(1));
    CHECK(model.isSelected(3));
    CHECK(model.isSelected(5));
    CHECK_FALSE(model.isSelected(2));
}

TEST_CASE("toggling adds then removes")
{
    PianoRollModel model;

    model.toggleSelection(7);
    CHECK(model.isSelected(7));

    model.toggleSelection(7);
    CHECK_FALSE(model.isSelected(7));
    CHECK(model.selection().empty());
}

TEST_CASE("pruning drops indices for notes that no longer exist")
{
    // A selection outlives the notes it referred to whenever something is
    // deleted; keeping stale indices would edit the wrong notes next time.
    PianoRollModel model;
    model.setSelection({0, 2, 4, 9});

    model.pruneSelection(5);

    CHECK(model.selection().size() == 3);
    CHECK_FALSE(model.isSelected(9));
}

// ── Phase 6 performance gate ──────────────────────────────────────────────────

TEST_CASE("culling 10,000 notes fits comfortably inside a 60 fps frame")
{
    // docs/ROADMAP.md Phase 6 requires 60 fps with 10,000 notes. The frame
    // budget is 16.6 ms; culling is what must not eat it. This measures the
    // arithmetic half of that requirement — the rendering half belongs to the
    // Metal layer and is not covered here.
    PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    notes.reserve(10000);

    for (int index = 0; index < 10000; ++index)
        notes.push_back(note(static_cast<Tick>(index) * 40,
                                      48 + (index % 36),
                                      120));

    std::vector<PianoRollModel::VisibleNote> visible;

    // Warm up, so the measurement is of the steady state rather than of the
    // first allocation.
    for (int frame = 0; frame < 10; ++frame)
        model.collectVisibleNotes(notes, visible);

    constexpr int frames = 240;   // four seconds of animation
    const auto    started = std::chrono::steady_clock::now();

    for (int frame = 0; frame < frames; ++frame) {
        auto viewport = model.viewport();
        viewport.firstTick = static_cast<Tick>(frame) * 60;   // scrolling
        model.setViewport(viewport);

        model.collectVisibleNotes(notes, visible);
    }

    const double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - started).count();
    const double perFrameMs = elapsed / frames * 1000.0;

    MESSAGE("cull of 10,000 notes: " << perFrameMs << " ms/frame");

    // A generous bar deliberately: this must hold on a loaded machine and in a
    // debug build, not only in ideal conditions. Anything near 16.6 ms would
    // mean culling alone had consumed the whole frame.
    CHECK(perFrameMs < 4.0);
}

TEST_CASE("culling does not allocate once the buffer has grown")
{
    // Rebuilding a fresh vector every frame is the difference between smooth
    // scrolling and a stutter each time the allocator decides to grow.
    PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    for (int index = 0; index < 2000; ++index)
        notes.push_back(note(static_cast<Tick>(index) * 40, 48 + (index % 36), 120));

    std::vector<PianoRollModel::VisibleNote> visible;

    for (int frame = 0; frame < 20; ++frame)
        model.collectVisibleNotes(notes, visible);

    engine::rt::resetViolations();
    {
        const engine::rt::ScopedRealtimeContext scope;
        for (int frame = 0; frame < 100; ++frame)
            model.collectVisibleNotes(notes, visible);
    }

    CHECK(engine::rt::allocationViolations() == 0);
}

TEST_CASE("hit testing 10,000 notes is fast enough for mouse tracking")
{
    PianoRollModel model = makeModel();

    std::vector<project::MidiEvent> notes;
    for (int index = 0; index < 10000; ++index)
        notes.push_back(note(static_cast<Tick>(index) * 40, 48 + (index % 36), 120));

    const auto started = std::chrono::steady_clock::now();

    volatile std::size_t sink = 0;
    for (int sample = 0; sample < 500; ++sample)
        sink = model.noteAtPoint(notes, static_cast<double>(sample % 1200), 300.0);
    (void)sink;

    const double perHitMs = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - started).count() / 500.0 * 1000.0;

    MESSAGE("hit test over 10,000 notes: " << perHitMs << " ms");
    CHECK(perHitMs < 2.0);
}

// ── Velocity lane ────────────────────────────────────────────────────────────
//
// The lane is the only way velocity is editable at all: the grid's vertical
// axis is already pitch, so a note's loudness has nowhere to live in it. What
// is tested here is the arithmetic the view draws and hit-tests from — where a
// bar sits for a given velocity, which velocity a click means, and which notes
// get a bar in the first place.

namespace {

project::MidiEvent noteAt(Tick tick, int key, int velocity, Tick duration = 240)
{
    project::MidiEvent event = note(tick, key, duration);
    event.value = velocity;
    return event;
}

/// The editing model with the lane open: 88 points of lane under 720 of grid.
PianoRollModel makeLaneModel()
{
    PianoRollModel model = makeModel();

    PianoRollModel::Viewport viewport = model.viewport();
    viewport.velocityLaneHeight = 88.0;
    model.setViewport(viewport);

    return model;
}

} // namespace

TEST_CASE("without a lane height there is no lane, and nothing pretends otherwise")
{
    const PianoRollModel model = makeModel();

    CHECK_FALSE(model.hasVelocityLane());

    // Every entry point agrees, so a caller needs one test and not four.
    CHECK_FALSE(model.isInVelocityLane(model.viewport().height + 10.0));
    CHECK_FALSE(model.isInVelocityLane(0.0));

    const NoteList notes{noteAt(0, 60, 100)};

    std::vector<PianoRollModel::VelocityBar> bars;
    model.collectVelocityBars(notes, bars);
    CHECK(bars.empty());

    CHECK(model.barAtPoint(notes, 0.0, model.viewport().height + 10.0) == PianoRollModel::noNote);
}

TEST_CASE("the lane occupies the band below the grid, and nothing above it")
{
    const PianoRollModel model = makeLaneModel();

    REQUIRE(model.hasVelocityLane());
    CHECK(model.velocityLaneTop() == doctest::Approx(720.0));
    CHECK(model.velocityLaneBottom() == doctest::Approx(808.0));

    CHECK_FALSE(model.isInVelocityLane(719.5));   // the last row of the grid
    CHECK(model.isInVelocityLane(720.0));         // the lane's first point
    CHECK(model.isInVelocityLane(807.5));
    CHECK_FALSE(model.isInVelocityLane(808.0));   // past the floor
}

TEST_CASE("velocity and lane position round-trip")
{
    const PianoRollModel model = makeLaneModel();

    for (const int velocity : {1, 20, 32, 64, 96, 110, 127})
        CHECK(model.yToVelocity(model.velocityToY(velocity)) == velocity);

    // Louder is higher, and the loudest still clears the lane's top edge so a
    // full bar cannot be mistaken for a clipped one.
    CHECK(model.velocityToY(127) < model.velocityToY(64));
    CHECK(model.velocityToY(127) > model.velocityLaneTop());
}

TEST_CASE("a drag past the lane pins rather than wraps")
{
    const PianoRollModel model = makeLaneModel();

    // Well above the lane, and well below it: a hand overshooting an 88-point
    // strip must not send the velocity to the other end of the range.
    CHECK(model.yToVelocity(-500.0) == 127);
    CHECK(model.yToVelocity(0.0) == 127);
    CHECK(model.yToVelocity(5000.0) == 1);

    // Zero is note-off and must never be reachable: a note carrying it is
    // silent with no visible reason.
    CHECK(model.yToVelocity(model.velocityLaneBottom()) == 1);
}

TEST_CASE("a bar's height is its velocity")
{
    const PianoRollModel model = makeLaneModel();

    const NoteList notes{noteAt(0, 60, 20), noteAt(480, 62, 120)};

    std::vector<PianoRollModel::VelocityBar> bars;
    model.collectVelocityBars(notes, bars);

    REQUIRE(bars.size() == 2);
    CHECK(bars[0].velocity == 20);
    CHECK(bars[1].velocity == 120);
    CHECK(bars[0].height < bars[1].height);

    // Every bar stands on the same floor, whatever its height.
    for (const auto& bar : bars)
        CHECK(bar.top + bar.height == doctest::Approx(model.velocityLaneBottom()));
}

TEST_CASE("a note that starts before the viewport gets no bar, even though it is drawn")
{
    PianoRollModel model = makeLaneModel();

    PianoRollModel::Viewport viewport = model.viewport();
    viewport.firstTick = ticksPerQuarterNote * 4;      // scrolled one bar in
    model.setViewport(viewport);

    // Four beats long, started a bar ago: still crossing the screen, but its
    // event is behind the left edge.
    const NoteList notes{noteAt(0, 60, 100, ticksPerQuarterNote * 8)};

    std::vector<PianoRollModel::VisibleNote> visible;
    model.collectVisibleNotes(notes, visible);
    CHECK(visible.size() == 1);        // the grid still draws it

    std::vector<PianoRollModel::VelocityBar> bars;
    model.collectVelocityBars(notes, bars);
    CHECK(bars.empty());               // the lane does not, because it could not be grabbed
}

TEST_CASE("the lane shows what the grid shows, and nothing else")
{
    const PianoRollModel model = makeLaneModel();   // keys 48..83, ticks 0..1920

    project::MidiEvent cc;
    cc.type = project::MidiEventType::controlChange;
    cc.tick = 0;
    cc.key  = 74;

    const NoteList notes{
        noteAt(0, 60, 100),                              // in view
        noteAt(240, 24, 100),                            // below the key range
        noteAt(240, 120, 100),                           // above the key range
        noteAt(ticksPerQuarterNote * 40, 60, 100),       // past the right edge
        cc,                                              // not a note at all
    };

    std::vector<PianoRollModel::VelocityBar> bars;
    model.collectVelocityBars(notes, bars);

    REQUIRE(bars.size() == 1);
    CHECK(bars[0].index == 0);
}

TEST_CASE("selection is carried into the bars, so the lane shows what an edit will hit")
{
    PianoRollModel model = makeLaneModel();

    const NoteList notes{noteAt(0, 60, 100), noteAt(480, 62, 100)};
    model.setSelection({1});

    std::vector<PianoRollModel::VelocityBar> bars;
    model.collectVelocityBars(notes, bars);

    REQUIRE(bars.size() == 2);
    CHECK_FALSE(bars[0].selected);
    CHECK(bars[1].selected);
}

TEST_CASE("the whole column is a target, not just the filled part of the bar")
{
    const PianoRollModel model = makeLaneModel();

    // Velocity 5: a stem barely three points tall. Requiring the user to hit
    // those three points would make the quietest notes the hardest to raise.
    const NoteList notes{noteAt(0, 60, 5)};

    const double column = model.tickToX(0) + PianoRollModel::velocityBarWidth * 0.5;

    CHECK(model.barAtPoint(notes, column, model.velocityLaneTop() + 2.0) == 0);
    CHECK(model.barAtPoint(notes, column, model.velocityLaneBottom() - 2.0) == 0);
}

TEST_CASE("a click outside a bar's column, or outside the lane, hits nothing")
{
    const PianoRollModel model = makeLaneModel();

    const NoteList notes{noteAt(0, 60, 100)};
    const double   inside = model.tickToX(0) + 1.0;

    CHECK(model.barAtPoint(notes, inside, 760.0) == 0);

    // Just past the stem's right edge.
    CHECK(model.barAtPoint(notes, model.tickToX(0) + PianoRollModel::velocityBarWidth + 1.0,
                           760.0) == PianoRollModel::noNote);

    // In the grid, where the same x is squarely inside the note itself.
    CHECK(model.barAtPoint(notes, inside, 100.0) == PianoRollModel::noNote);
}

TEST_CASE("where two bars share a column, the one drawn on top is picked")
{
    const PianoRollModel model = makeLaneModel();

    // A chord: same tick, different keys, so the stems land on each other.
    const NoteList notes{noteAt(0, 60, 100), noteAt(0, 64, 100), noteAt(0, 67, 100)};

    const double column = model.tickToX(0) + 1.0;

    CHECK(model.barAtPoint(notes, column, 760.0) == 2);
}
