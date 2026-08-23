#pragma once

#include "engine/automation/AutomationSequence.h"
#include "engine/core/Time.h"
#include "project/Model.h"

#include <cstddef>
#include <vector>

namespace incdaw::app {

using engine::Tick;
using project::AutomationCurve;
using project::AutomationPoint;

/// The automation editor's geometry, selection and edits, with no drawing.
///
/// The fourth model of this shape after ChannelRackModel, PianoRollModel and
/// PlaylistModel, and for the same reason: where a point sits, what the mouse
/// is over, and what a gesture does to a lane are all arithmetic, and
/// arithmetic is testable without a window.
///
/// Every edit is a pure function from one point vector to the next, which is
/// what lets all of them — draw, erase, drag, curve, tension, scale, paste —
/// go through the one `SetAutomationPointsCommand` the automation subsystem
/// already had. There is no per-gesture command and no per-gesture undo path.
///
/// The curve the editor DRAWS is evaluated by engine::AutomationSequence, the
/// same class the audio thread reads. A second interpolation here would drift
/// from the one being played, and the difference would be invisible until it
/// was audible.
class AutomationEditorModel {
public:
    struct Viewport {
        Tick   firstTick    = 0;
        Tick   visibleTicks = engine::ticksPerQuarterNote * 4 * 8;
        double width        = 0.0;
        double height       = 0.0;

        /// The band above the curve where bars are numbered.
        double rulerHeight  = 0.0;
    };

    struct Rect {
        double x = 0.0;
        double y = 0.0;
        double width = 0.0;
        double height = 0.0;

        [[nodiscard]] bool contains(double pointX, double pointY) const noexcept
        {
            return pointX >= x && pointX < x + width && pointY >= y && pointY < y + height;
        }
    };

    static constexpr std::size_t noPoint = static_cast<std::size_t>(-1);

    /// Grab radius around a point, in points. Generous on purpose: a four-pixel
    /// dot is a target nobody can hit twice in a row.
    static constexpr double handleRadius = 5.0;

    // ── Viewport ────────────────────────────────────────────────────────────

    void setViewport(const Viewport& viewport) noexcept;
    [[nodiscard]] const Viewport& viewport() const noexcept { return viewport_; }

    [[nodiscard]] double pointsPerTick() const noexcept
    {
        return viewport_.visibleTicks > 0
                   ? viewport_.width / static_cast<double>(viewport_.visibleTicks)
                   : 0.0;
    }

    [[nodiscard]] double tickToX(Tick tick) const noexcept
    {
        return static_cast<double>(tick - viewport_.firstTick) * pointsPerTick();
    }

    [[nodiscard]] Tick xToTick(double x) const noexcept;

    /// The curve's own band, below the ruler.
    [[nodiscard]] double curveTop() const noexcept { return viewport_.rulerHeight; }
    [[nodiscard]] double curveHeight() const noexcept
    {
        return std::max(0.0, viewport_.height - viewport_.rulerHeight);
    }

    /// Value 0..1 to y. One at the top, zero at the bottom — a fader read the
    /// other way round is a fader nobody trusts.
    [[nodiscard]] double valueToY(double value) const noexcept;
    [[nodiscard]] double yToValue(double y) const noexcept;

    [[nodiscard]] Rect pointRect(const AutomationPoint& point) const noexcept;

    // ── The curve ───────────────────────────────────────────────────────────

    /// The lane's value at `tick`, through the engine's own evaluator.
    [[nodiscard]] static double valueAt(const std::vector<AutomationPoint>& points,
                                        Tick tick);

    /// Samples the curve across the viewport, one value per x step, for
    /// drawing. `out` is a caller-owned buffer reused across frames.
    void collectCurve(const std::vector<AutomationPoint>& points,
                      std::vector<double>& out, double xStep = 2.0) const;

    // ── Hit testing ─────────────────────────────────────────────────────────

    [[nodiscard]] std::size_t pointAt(const std::vector<AutomationPoint>& points,
                                      double x, double y) const noexcept;

    /// The segment a click at `x` falls in, named by the index of the point
    /// that STARTS it — which is the point whose curve and tension the segment
    /// obeys. `noPoint` before the first point or after the last.
    [[nodiscard]] std::size_t segmentAt(const std::vector<AutomationPoint>& points,
                                        double x) const noexcept;

    void pointsInRectangle(const std::vector<AutomationPoint>& points,
                           double x, double y, double width, double height,
                           std::vector<std::size_t>& out) const;

    // ── Grid ────────────────────────────────────────────────────────────────

    void setSnap(Tick snap) noexcept { snap_ = snap > 0 ? snap : 0; }
    [[nodiscard]] Tick snap() const noexcept { return snap_; }
    [[nodiscard]] Tick snapTick(Tick tick) const noexcept;

    // ── Selection ───────────────────────────────────────────────────────────
    //
    // Held as indices into the lane's point vector, and therefore only valid
    // between edits: every edit replaces the vector, so the caller re-selects
    // from what the edit returned. Ids would be the alternative, and an id per
    // automation point is a lot of bookkeeping for a thing whose whole identity
    // is a position.

    void setSelection(std::vector<std::size_t> points);
    void addToSelection(std::size_t index);
    void toggleSelection(std::size_t index);
    void clearSelection() noexcept { selection_.clear(); }

    [[nodiscard]] bool isSelected(std::size_t index) const noexcept;
    [[nodiscard]] const std::vector<std::size_t>& selection() const noexcept
    {
        return selection_;
    }

    /// Drops indices past the end, after an edit shortened the lane.
    void pruneSelection(std::size_t pointCount);

    // ── Edits ───────────────────────────────────────────────────────────────
    //
    // Each returns the lane's next point vector. None of them touches the
    // project: the caller hands the result to SetAutomationPointsCommand,
    // which is what makes every one of them exactly undoable.

    /// Adds a point, replacing one already at that tick — two points on one
    /// tick is a segment of zero length, which the evaluator cannot read.
    [[nodiscard]] static std::vector<AutomationPoint> withPointAdded(
        std::vector<AutomationPoint> points, Tick tick, double value,
        AutomationCurve curve = AutomationCurve::linear);

    [[nodiscard]] static std::vector<AutomationPoint> withPointsRemoved(
        std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices);

    /// Moves points in time and value, clamped to tick zero and to 0..1.
    /// Clamped for the whole set, so a dragged group keeps its shape.
    [[nodiscard]] static std::vector<AutomationPoint> withPointsMoved(
        std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices,
        Tick tickDelta, double valueDelta);

    [[nodiscard]] static std::vector<AutomationPoint> withCurve(
        std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices,
        AutomationCurve curve);

    [[nodiscard]] static std::vector<AutomationPoint> withTension(
        std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices,
        double tension);

    /// Scales the selection's span in time about `anchorTick`. A factor of 1
    /// changes nothing; 0.5 halves the distances between them.
    [[nodiscard]] static std::vector<AutomationPoint> withTimeScaled(
        std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices,
        double factor, Tick anchorTick);

    /// Scales the selection's values about `anchorValue`, clamped to 0..1.
    [[nodiscard]] static std::vector<AutomationPoint> withValueScaled(
        std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices,
        double factor, double anchorValue);

    /// The selection, rebased so its earliest point sits at tick zero — the
    /// clipboard's own form, so a paste is a translation and nothing else.
    [[nodiscard]] static std::vector<AutomationPoint> copyOf(
        const std::vector<AutomationPoint>& points,
        const std::vector<std::size_t>& indices);

    /// Drops `clipboard` in at `atTick`, replacing whatever it lands on. The
    /// clipboard may come from another lane: a lane is a list of normalised
    /// points and nothing else, which is exactly why one can be pasted into
    /// another (CLAUDE.md §10).
    [[nodiscard]] static std::vector<AutomationPoint> withPasted(
        std::vector<AutomationPoint> points,
        const std::vector<AutomationPoint>& clipboard, Tick atTick);

    /// Where `clipboard` would land, so the caller can select it afterwards.
    [[nodiscard]] static std::vector<std::size_t> pastedIndices(
        const std::vector<AutomationPoint>& points,
        const std::vector<AutomationPoint>& clipboard, Tick atTick);

private:
    Viewport                 viewport_;
    Tick                     snap_ = engine::ticksPerQuarterNote / 4;
    std::vector<std::size_t> selection_;
};

} // namespace incdaw::app
