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

    project::Pattern pattern;
    pattern.events.push_back(note(0, 60));

    std::vector<PianoRollModel::VisibleNote> visible;
    model.collectVisibleNotes(pattern, visible);
    CHECK(visible.empty());
}

// ── Culling ───────────────────────────────────────────────────────────────────

TEST_CASE("only notes intersecting the viewport are collected")
{
    const PianoRollModel model = makeModel();

    project::Pattern pattern;
    pattern.events.push_back(note(0, 60));                              // visible
    pattern.events.push_back(note(ticksPerQuarterNote * 2, 72));        // visible
    pattern.events.push_back(note(ticksPerQuarterNote * 8, 60));        // right of view
    pattern.events.push_back(note(0, 20));                              // below view
    pattern.events.push_back(note(0, 100));                             // above view

    std::vector<PianoRollModel::VisibleNote> visible;
    model.collectVisibleNotes(pattern, visible);

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

    project::Pattern pattern;
    pattern.events.push_back(note(0, 60, ticksPerQuarterNote * 8));   // spans the viewport

    std::vector<PianoRollModel::VisibleNote> visible;
    model.collectVisibleNotes(pattern, visible);

    REQUIRE(visible.size() == 1);
    CHECK(visible[0].x < 0.0);   // starts off the left edge
}

TEST_CASE("non-note events are not drawn as notes")
{
    const PianoRollModel model = makeModel();

    project::Pattern pattern;
    project::MidiEvent cc;
    cc.type = project::MidiEventType::controlChange;
    cc.tick = 0;
    pattern.events.push_back(cc);

    std::vector<PianoRollModel::VisibleNote> visible;
    model.collectVisibleNotes(pattern, visible);
    CHECK(visible.empty());
}

TEST_CASE("selection state is carried into the visible list")
{
    PianoRollModel model = makeModel();

    project::Pattern pattern;
    pattern.events.push_back(note(0, 60));
    pattern.events.push_back(note(480, 62));

    model.setSelection({1});

    std::vector<PianoRollModel::VisibleNote> visible;
    model.collectVisibleNotes(pattern, visible);

    REQUIRE(visible.size() == 2);
    CHECK_FALSE(visible[0].selected);
    CHECK(visible[1].selected);
}

// ── Hit testing ───────────────────────────────────────────────────────────────

TEST_CASE("clicking inside a note selects it, and empty space selects nothing")
{
    const PianoRollModel model = makeModel();

    project::Pattern pattern;
    pattern.events.push_back(note(ticksPerQuarterNote, 60, 480));

    const double x = model.tickToX(ticksPerQuarterNote + 100);
    const double y = model.keyToY(60) + 2.0;

    CHECK(model.noteAtPoint(pattern, x, y) == 0);

    CHECK(model.noteAtPoint(pattern, x, model.keyToY(61) + 2.0) == PianoRollModel::noNote);
    CHECK(model.noteAtPoint(pattern, model.tickToX(0) + 1.0, y) == PianoRollModel::noNote);
}

TEST_CASE("where notes overlap, the one drawn on top is picked")
{
    const PianoRollModel model = makeModel();

    project::Pattern pattern;
    pattern.events.push_back(note(0, 60, 960));
    pattern.events.push_back(note(0, 60, 960));   // same place, drawn later

    const double x = model.tickToX(100);
    const double y = model.keyToY(60) + 2.0;

    CHECK(model.noteAtPoint(pattern, x, y) == 1);
}

TEST_CASE("the right edge of a note is a resize handle, the middle is not")
{
    const PianoRollModel model = makeModel();

    project::Pattern pattern;
    pattern.events.push_back(note(0, 60, ticksPerQuarterNote));

    const double y     = model.keyToY(60) + 2.0;
    const double right = model.tickToX(ticksPerQuarterNote);

    CHECK(model.isOverResizeHandle(pattern, 0, right - 2.0, y));
    CHECK_FALSE(model.isOverResizeHandle(pattern, 0, model.tickToX(100), y));
    CHECK_FALSE(model.isOverResizeHandle(pattern, 0, right - 2.0, model.keyToY(61) + 2.0));
}

TEST_CASE("a very short note still leaves room to grab and move it")
{
    // If the handle covered the whole note there would be no way to drag it.
    const PianoRollModel model = makeModel();

    project::Pattern pattern;
    pattern.events.push_back(note(0, 60, 4));

    const double y    = model.keyToY(60) + 2.0;
    const double left = model.tickToX(0);

    CHECK_FALSE(model.isOverResizeHandle(pattern, 0, left + 0.1, y));
}

// ── Box selection ─────────────────────────────────────────────────────────────

TEST_CASE("box selection catches notes that merely intersect the rectangle")
{
    const PianoRollModel model = makeModel();

    project::Pattern pattern;
    pattern.events.push_back(note(0, 60, ticksPerQuarterNote * 3));   // long, extends past the box
    pattern.events.push_back(note(0, 72, 120));                        // outside vertically

    std::vector<std::size_t> hits;
    model.notesInRectangle(pattern, 0.0, model.keyToY(60), 50.0, model.keyHeight(), hits);

    REQUIRE(hits.size() == 1);
    CHECK(hits[0] == 0);
}

TEST_CASE("a rectangle dragged up and to the left works the same as one dragged down and right")
{
    const PianoRollModel model = makeModel();

    project::Pattern pattern;
    pattern.events.push_back(note(ticksPerQuarterNote, 60));

    const double x = model.tickToX(ticksPerQuarterNote) + 5.0;
    const double y = model.keyToY(60) + 5.0;

    std::vector<std::size_t> forward;
    std::vector<std::size_t> backward;

    model.notesInRectangle(pattern, x - 40.0, y - 30.0, 80.0, 60.0, forward);
    model.notesInRectangle(pattern, x + 40.0, y + 30.0, -80.0, -60.0, backward);

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

    project::Pattern pattern;
    pattern.events.reserve(10000);

    for (int index = 0; index < 10000; ++index)
        pattern.events.push_back(note(static_cast<Tick>(index) * 40,
                                      48 + (index % 36),
                                      120));

    std::vector<PianoRollModel::VisibleNote> visible;

    // Warm up, so the measurement is of the steady state rather than of the
    // first allocation.
    for (int frame = 0; frame < 10; ++frame)
        model.collectVisibleNotes(pattern, visible);

    constexpr int frames = 240;   // four seconds of animation
    const auto    started = std::chrono::steady_clock::now();

    for (int frame = 0; frame < frames; ++frame) {
        auto viewport = model.viewport();
        viewport.firstTick = static_cast<Tick>(frame) * 60;   // scrolling
        model.setViewport(viewport);

        model.collectVisibleNotes(pattern, visible);
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

    project::Pattern pattern;
    for (int index = 0; index < 2000; ++index)
        pattern.events.push_back(note(static_cast<Tick>(index) * 40, 48 + (index % 36), 120));

    std::vector<PianoRollModel::VisibleNote> visible;

    for (int frame = 0; frame < 20; ++frame)
        model.collectVisibleNotes(pattern, visible);

    engine::rt::resetViolations();
    {
        const engine::rt::ScopedRealtimeContext scope;
        for (int frame = 0; frame < 100; ++frame)
            model.collectVisibleNotes(pattern, visible);
    }

    CHECK(engine::rt::allocationViolations() == 0);
}

TEST_CASE("hit testing 10,000 notes is fast enough for mouse tracking")
{
    PianoRollModel model = makeModel();

    project::Pattern pattern;
    for (int index = 0; index < 10000; ++index)
        pattern.events.push_back(note(static_cast<Tick>(index) * 40, 48 + (index % 36), 120));

    const auto started = std::chrono::steady_clock::now();

    volatile std::size_t sink = 0;
    for (int sample = 0; sample < 500; ++sample)
        sink = model.noteAtPoint(pattern, static_cast<double>(sample % 1200), 300.0);
    (void)sink;

    const double perHitMs = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - started).count() / 500.0 * 1000.0;

    MESSAGE("hit test over 10,000 notes: " << perHitMs << " ms");
    CHECK(perHitMs < 2.0);
}
