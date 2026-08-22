#pragma once

#include "project/Model.h"

#include <cstddef>
#include <vector>

namespace incdaw::app {

using project::MidiEvent;
using project::Pattern;

/// One channel's notes within a pattern — what the editor is looking at.
using NoteList = std::vector<MidiEvent>;
using project::Tick;

/// The Piano Roll's geometry and hit-testing, with no drawing in it.
///
/// Separated from the renderer deliberately. Everything that decides *what is
/// on screen and what the mouse is over* is arithmetic, and arithmetic is
/// testable headlessly — including the performance requirement. The Metal layer
/// then only has to draw the list this produces.
///
/// docs/ROADMAP.md Phase 6 requires 60 fps with 10,000 notes. That budget is
/// spent here, in culling, long before a single triangle is submitted.
class PianoRollModel {
public:
    /// A note that intersects the viewport, with its screen rectangle.
    struct VisibleNote {
        std::size_t index = 0;      ///< index into the channel's event vector
        double      x = 0.0;
        double      y = 0.0;
        double      width = 0.0;
        double      height = 0.0;
        int         key = 60;
        int         velocity = 100;
        bool        selected = false;
    };

    /// One note's bar in the velocity lane.
    struct VelocityBar {
        std::size_t index = 0;
        double x = 0.0;         ///< left edge, aligned to the note's start
        double width = 0.0;
        double top = 0.0;       ///< the bar runs from here down to the lane's floor
        double height = 0.0;
        int    velocity = 100;
        bool   selected = false;
    };

    struct Viewport {
        Tick   firstTick   = 0;
        Tick   visibleTicks = incdaw::engine::ticksPerQuarterNote * 4;   ///< one 4/4 bar
        int    lowestKey   = 36;
        int    visibleKeys = 36;

        double width  = 1000.0;   ///< in points

        /// The numbered band above the grid, where bars are counted. Zero
        /// leaves the grid at the top, which is what the geometry tests
        /// describe; the view gives it a real height.
        double rulerHeight = 0.0;

        /// The NOTE GRID's height — not the view's, and not counting the ruler
        /// above it or the velocity lane below. Everything that draws or
        /// hit-tests a note measures against this and is unaffected by either.
        double height = 600.0;

        /// Height of the velocity lane below the grid, in points. Zero hides
        /// it, which is the default: the lane is a mode, not furniture.
        double velocityLaneHeight = 0.0;
    };

    void setViewport(const Viewport& viewport) noexcept { viewport_ = viewport; }
    [[nodiscard]] const Viewport& viewport() const noexcept { return viewport_; }

    /// Horizontal zoom, in points per tick.
    [[nodiscard]] double pointsPerTick() const noexcept
    {
        return viewport_.visibleTicks > 0
                   ? viewport_.width / static_cast<double>(viewport_.visibleTicks)
                   : 0.0;
    }

    /// Vertical zoom: the height of one key row, in points.
    [[nodiscard]] double keyHeight() const noexcept
    {
        return viewport_.visibleKeys > 0 ? viewport_.height / static_cast<double>(viewport_.visibleKeys) : 0.0;
    }

    // ── Coordinate conversion ───────────────────────────────────────────────

    [[nodiscard]] double tickToX(Tick tick) const noexcept
    {
        return static_cast<double>(tick - viewport_.firstTick) * pointsPerTick();
    }

    [[nodiscard]] Tick xToTick(double x) const noexcept
    {
        const double scale = pointsPerTick();
        return scale > 0.0 ? viewport_.firstTick + static_cast<Tick>(x / scale + 0.5) : viewport_.firstTick;
    }

    /// The band above the grid where bars are numbered.
    [[nodiscard]] double gridTop() const noexcept { return viewport_.rulerHeight; }

    [[nodiscard]] bool hasRuler() const noexcept { return viewport_.rulerHeight > 0.0; }

    /// True for points in the ruler. A click there is not an edit — it must
    /// never draw a note, which is what it would do if the band were simply
    /// the top of the grid.
    [[nodiscard]] bool isInRuler(double y) const noexcept
    {
        return hasRuler() && y >= 0.0 && y < viewport_.rulerHeight;
    }

    /// Higher keys are drawn higher on screen, so y increases as pitch falls.
    [[nodiscard]] double keyToY(int key) const noexcept
    {
        return gridTop()
             + static_cast<double>(viewport_.lowestKey + viewport_.visibleKeys - 1 - key)
               * keyHeight();
    }

    [[nodiscard]] int yToKey(double y) const noexcept
    {
        const double height = keyHeight();
        if (height <= 0.0)
            return viewport_.lowestKey;

        return viewport_.lowestKey + viewport_.visibleKeys - 1
             - static_cast<int>((y - gridTop()) / height);
    }

    // ── Culling ─────────────────────────────────────────────────────────────

    /// Fills `out` with the notes intersecting the viewport.
    ///
    /// `out` is a caller-owned buffer that is reused across frames, so a steady
    /// state costs no allocation at all. Building a fresh vector per frame is
    /// the difference between smooth scrolling and a stutter every time the
    /// allocator decides to grow.
    /// `append` keeps what `out` already holds, which is how the ghosts of
    /// several other channels are gathered into one list without a buffer per
    /// channel.
    void collectVisibleNotes(const NoteList& notes, std::vector<VisibleNote>& out,
                             bool append = false) const;

    // ── Hit testing ─────────────────────────────────────────────────────────

    static constexpr std::size_t noNote = static_cast<std::size_t>(-1);

    /// Topmost note under a point, or `noNote`.
    ///
    /// Searches back to front so that the note drawn last — the one visually on
    /// top where they overlap — is the one picked up.
    [[nodiscard]] std::size_t noteAtPoint(const NoteList& notes, double x, double y) const;

    /// True when the point is within the grab zone at a note's right edge,
    /// where dragging resizes rather than moves.
    [[nodiscard]] bool isOverResizeHandle(const NoteList& notes, std::size_t index,
                                          double x, double y) const;

    /// Notes intersecting a rectangle, for box and lasso selection.
    void notesInRectangle(const NoteList& notes, double x, double y, double width, double height,
                          std::vector<std::size_t>& out) const;

    // ── Grid ────────────────────────────────────────────────────────────────

    void setSnap(Tick snap) noexcept { snap_ = snap > 0 ? snap : 0; }
    [[nodiscard]] Tick snap() const noexcept { return snap_; }

    /// Rounds to the nearest grid line. Snap of zero means free placement.
    [[nodiscard]] Tick snapTick(Tick tick) const noexcept;

    // ── Selection ───────────────────────────────────────────────────────────

    void setSelection(std::vector<std::size_t> indices);
    void addToSelection(std::size_t index);
    void toggleSelection(std::size_t index);
    void clearSelection() noexcept { selection_.clear(); }

    [[nodiscard]] bool isSelected(std::size_t index) const noexcept;
    [[nodiscard]] const std::vector<std::size_t>& selection() const noexcept { return selection_; }

    /// Drops indices that no longer exist, after notes have been deleted.
    void pruneSelection(std::size_t noteCount);

    /// Grab zone at a note's right edge, in points.
    static constexpr double resizeHandleWidth = 6.0;

    // ── Velocity lane ───────────────────────────────────────────────────────
    //
    // FL Studio calls this the event lane, and every piano roll has some form
    // of it for the same reason: velocity is the one note property that changes
    // how a part FEELS rather than what it plays, and it is unreachable through
    // the grid because the grid's vertical axis is already pitch.
    //
    // Only the arithmetic lives here. The lane is drawn by the view out of the
    // list `collectVelocityBars` produces, exactly as the grid is.

    /// Width of a velocity bar, in points.
    ///
    /// Fixed rather than taken from the note's length: a whole-bar note would
    /// otherwise get a bar wide enough to bury every shorter note beside it,
    /// and the lane is meant to read as a row of stems at note starts.
    static constexpr double velocityBarWidth = 9.0;

    /// Gap above the tallest bar, so a velocity of 127 does not touch the
    /// lane's top edge and become indistinguishable from a clipped one.
    static constexpr double velocityLanePadding = 4.0;

    [[nodiscard]] bool hasVelocityLane() const noexcept
    {
        return viewport_.velocityLaneHeight > 0.0;
    }

    [[nodiscard]] double velocityLaneTop() const noexcept
    {
        return gridTop() + viewport_.height;
    }

    [[nodiscard]] double velocityLaneBottom() const noexcept
    {
        return velocityLaneTop() + viewport_.velocityLaneHeight;
    }

    /// False whenever the lane is hidden, so callers need only one test.
    [[nodiscard]] bool isInVelocityLane(double y) const noexcept
    {
        return hasVelocityLane() && y >= velocityLaneTop() && y < velocityLaneBottom();
    }

    /// The top of the bar drawn for `velocity`. Velocity is clamped to 1..127:
    /// zero is note-off, and a note in a pattern must never carry it.
    [[nodiscard]] double velocityToY(int velocity) const noexcept;

    /// The velocity a point in the lane means, clamped to 1..127. Points above
    /// the lane read as 127 and below it as 1, so a drag that overshoots the
    /// lane pins rather than jumps.
    [[nodiscard]] int yToVelocity(double y) const noexcept;

    /// Fills `out` with a bar for every note whose START is in view.
    ///
    /// Deliberately stricter than note culling, which keeps a long note that
    /// began before the viewport: that note's event is at its start, so its bar
    /// would sit off the left edge where it can be seen and not grabbed. A bar
    /// is either fully addressable or absent.
    void collectVelocityBars(const NoteList& notes, std::vector<VelocityBar>& out) const;

    /// The note whose bar is under a point, or `noNote`. Returns `noNote` for
    /// any point outside the lane, so one call answers "is this a lane edit?".
    [[nodiscard]] std::size_t barAtPoint(const NoteList& notes, double x, double y) const;

private:
    Viewport                 viewport_;
    Tick                     snap_ = 0;
    std::vector<std::size_t> selection_;
};

} // namespace incdaw::app
