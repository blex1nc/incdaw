#pragma once

#include "app/MusicTheory.h"
#include "engine/core/Time.h"

#include <cstddef>

namespace incdaw::app {

using engine::Tick;

/// The Piano Roll header's geometry and hit testing, with no drawing in it.
///
/// The third model of this shape, after ChannelRackModel and PianoRollModel,
/// and for the same reason: where a control sits and what the mouse is over is
/// arithmetic, and arithmetic is testable without a window.
///
/// The header exists because three things the editor already does were
/// reachable only from the keyboard, or not at all: the grid resolution was
/// fixed at a sixteenth, and the key signature the scale highlighting and the
/// nudge tool both read was fixed at C major. A control surface that shows what
/// the grid is set to is the difference between an editor a stranger can drive
/// and one that has to be explained.
class PianoRollHeaderModel {
public:
    struct Layout {
        double height       = 28.0;
        double padding      = 8.0;
        double gap          = 6.0;

        /// The three readouts — snap, key, scale — are the same width, because
        /// they are read as one group and a ragged group reads as three.
        double readoutWidth = 92.0;

        /// GHOST and VEL sit at the trailing edge, away from the readouts: one
        /// group answers "what is the grid", the other "what can I see".
        double toggleWidth  = 54.0;
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

    enum class Zone { none, snap, key, scale, ghosts, velocityLane };

    // ── Snap ────────────────────────────────────────────────────────────────

    /// Grid resolutions, coarsest first. "Off" is last rather than first
    /// because it is the exception: a piano roll snaps unless told not to.
    enum class Snap { bar, half, quarter, eighth, sixteenth, thirtySecond, off };

    static constexpr std::size_t snapCount = 7;

    [[nodiscard]] static Snap snapAt(std::size_t index) noexcept
    {
        return index < snapCount ? static_cast<Snap>(index) : Snap::sixteenth;
    }

    /// Ticks per grid division, or 0 for no snapping — which is exactly what
    /// PianoRollModel::setSnap means by zero, so the header hands its value
    /// straight over without a second vocabulary in between.
    [[nodiscard]] static Tick ticksFor(Snap snap) noexcept
    {
        using engine::ticksPerQuarterNote;

        switch (snap) {
            case Snap::bar:          return ticksPerQuarterNote * 4;
            case Snap::half:         return ticksPerQuarterNote * 2;
            case Snap::quarter:      return ticksPerQuarterNote;
            case Snap::eighth:       return ticksPerQuarterNote / 2;
            case Snap::sixteenth:    return ticksPerQuarterNote / 4;
            case Snap::thirtySecond: return ticksPerQuarterNote / 8;
            case Snap::off:          return 0;
        }

        return ticksPerQuarterNote / 4;
    }

    /// The setting a tick value came from, so the header can show what the
    /// editor is actually snapping to rather than a remembered menu choice.
    /// A value that is not one of the offered divisions reads as the nearest
    /// one at or below it; zero is off.
    [[nodiscard]] static Snap snapForTicks(Tick ticks) noexcept
    {
        if (ticks <= 0)
            return Snap::off;

        Snap best = Snap::thirtySecond;
        for (std::size_t index = 0; index < snapCount; ++index) {
            const Snap candidate = snapAt(index);
            if (candidate != Snap::off && ticksFor(candidate) <= ticks) {
                best = candidate;
                break;
            }
        }

        return best;
    }

    [[nodiscard]] static const char* snapName(Snap snap) noexcept
    {
        switch (snap) {
            case Snap::bar:          return "1 bar";
            case Snap::half:         return "1/2";
            case Snap::quarter:      return "1/4";
            case Snap::eighth:       return "1/8";
            case Snap::sixteenth:    return "1/16";
            case Snap::thirtySecond: return "1/32";
            case Snap::off:          return "Off";
        }

        return "1/16";
    }

    // ── Scales ──────────────────────────────────────────────────────────────

    static constexpr std::size_t scaleCount = 3;

    [[nodiscard]] static music::Scale scaleAt(std::size_t index) noexcept
    {
        return index < scaleCount ? static_cast<music::Scale>(index) : music::Scale::major;
    }

    [[nodiscard]] static const char* scaleName(music::Scale scale) noexcept
    {
        switch (scale) {
            case music::Scale::major:         return "Major";
            case music::Scale::naturalMinor:  return "Minor";
            case music::Scale::harmonicMinor: return "Harm min";
        }

        return "Major";
    }

    // ── Geometry ────────────────────────────────────────────────────────────

    void setLayout(const Layout& layout) noexcept { layout_ = layout; }
    [[nodiscard]] const Layout& layout() const noexcept { return layout_; }

    [[nodiscard]] Rect snapRect() const noexcept { return readout(0); }
    [[nodiscard]] Rect keyRect() const noexcept { return readout(1); }
    [[nodiscard]] Rect scaleRect() const noexcept { return readout(2); }

    /// `width` is the view's: the toggles are measured from the trailing edge,
    /// so they stay where the eye last saw them when the window is resized.
    [[nodiscard]] Rect ghostsRect(double width) const noexcept { return toggle(width, 1); }
    [[nodiscard]] Rect velocityLaneRect(double width) const noexcept { return toggle(width, 0); }

    [[nodiscard]] Zone hitTest(double x, double y, double width) const noexcept
    {
        if (y < 0.0 || y >= layout_.height)
            return Zone::none;

        if (snapRect().contains(x, y))
            return Zone::snap;
        if (keyRect().contains(x, y))
            return Zone::key;
        if (scaleRect().contains(x, y))
            return Zone::scale;
        if (ghostsRect(width).contains(x, y))
            return Zone::ghosts;
        if (velocityLaneRect(width).contains(x, y))
            return Zone::velocityLane;

        return Zone::none;
    }

private:
    [[nodiscard]] Rect readout(int index) const noexcept
    {
        const double pitch = layout_.readoutWidth + layout_.gap;

        return {layout_.padding + static_cast<double>(index) * pitch, inset(),
                layout_.readoutWidth, barHeight()};
    }

    /// `fromEnd` counts inward from the trailing edge: 0 is the last control.
    [[nodiscard]] Rect toggle(double width, int fromEnd) const noexcept
    {
        const double pitch = layout_.toggleWidth + layout_.gap;

        return {width - layout_.padding - static_cast<double>(fromEnd + 1) * pitch + layout_.gap,
                inset(), layout_.toggleWidth, barHeight()};
    }

    [[nodiscard]] double inset() const noexcept { return 4.0; }
    [[nodiscard]] double barHeight() const noexcept { return layout_.height - 2.0 * inset(); }

    Layout layout_;
};

} // namespace incdaw::app
