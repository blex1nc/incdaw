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

    struct Viewport {
        Tick   firstTick   = 0;
        Tick   visibleTicks = incdaw::engine::ticksPerQuarterNote * 4;   ///< one 4/4 bar
        int    lowestKey   = 36;
        int    visibleKeys = 36;

        double width  = 1000.0;   ///< in points
        double height = 600.0;
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

    /// Higher keys are drawn higher on screen, so y increases as pitch falls.
    [[nodiscard]] double keyToY(int key) const noexcept
    {
        return static_cast<double>(viewport_.lowestKey + viewport_.visibleKeys - 1 - key) * keyHeight();
    }

    [[nodiscard]] int yToKey(double y) const noexcept
    {
        const double height = keyHeight();
        if (height <= 0.0)
            return viewport_.lowestKey;

        return viewport_.lowestKey + viewport_.visibleKeys - 1 - static_cast<int>(y / height);
    }

    // ── Culling ─────────────────────────────────────────────────────────────

    /// Fills `out` with the notes intersecting the viewport.
    ///
    /// `out` is a caller-owned buffer that is reused across frames, so a steady
    /// state costs no allocation at all. Building a fresh vector per frame is
    /// the difference between smooth scrolling and a stutter every time the
    /// allocator decides to grow.
    void collectVisibleNotes(const NoteList& notes, std::vector<VisibleNote>& out) const;

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

private:
    Viewport                 viewport_;
    Tick                     snap_ = 0;
    std::vector<std::size_t> selection_;
};

} // namespace incdaw::app
