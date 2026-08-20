#pragma once

#include "project/Model.h"

#include <cstddef>

namespace incdaw::app {

using project::Pattern;
using project::Tick;

/// The Channel Rack's geometry and hit testing, with no drawing in it.
///
/// Same split as app::PianoRollModel and for the same reason: everything that
/// decides *where a row is and what the mouse is over* is arithmetic, and
/// arithmetic is testable without a window. The AppKit view then only draws the
/// rectangles this hands it and turns hits into commands.
class ChannelRackModel {
public:
    struct Layout {
        double rowHeight   = 34.0;
        double rowGap      = 3.0;

        /// Name, mute, solo and volume live left of the step grid.
        double headerWidth = 250.0;

        double stepWidth   = 26.0;
        double stepGap     = 3.0;

        double swatchWidth = 10.0;
        double buttonWidth = 20.0;
        double volumeWidth = 74.0;
        double padding     = 8.0;
    };

    struct Rect {
        double x      = 0.0;
        double y      = 0.0;
        double width  = 0.0;
        double height = 0.0;

        [[nodiscard]] bool contains(double pointX, double pointY) const noexcept
        {
            return pointX >= x && pointX < x + width && pointY >= y && pointY < y + height;
        }
    };

    enum class Zone { none, name, mute, solo, volume, step };

    static constexpr std::size_t noRow = static_cast<std::size_t>(-1);

    struct Hit {
        Zone        zone = Zone::none;
        std::size_t row  = noRow;
        int         step = -1;
    };

    void setLayout(const Layout& layout) noexcept { layout_ = layout; }
    [[nodiscard]] const Layout& layout() const noexcept { return layout_; }

    /// Grid resolution. One step is a sixteenth by default, which is what a
    /// step sequencer means by "a step" unless told otherwise.
    void setStepTicks(Tick ticks) noexcept;
    [[nodiscard]] Tick stepTicks() const noexcept { return stepTicks_; }

    /// Horizontal scroll, in steps.
    void setFirstStep(int step) noexcept { firstStep_ = step > 0 ? step : 0; }
    [[nodiscard]] int firstStep() const noexcept { return firstStep_; }

    /// Steps a pattern is long. A pattern that does not divide evenly gets the
    /// partial step too, because the notes inside it are real.
    [[nodiscard]] int stepCount(const Pattern& pattern) const noexcept;

    [[nodiscard]] Tick tickForStep(int step) const noexcept;

    /// How many steps fit in `width` points of grid, scroll included.
    [[nodiscard]] int visibleStepCount(double width) const noexcept;

    // ── Geometry, top-down: row 0 is at y = 0 ───────────────────────────────

    [[nodiscard]] double rowY(std::size_t row) const noexcept;
    [[nodiscard]] double rowPitch() const noexcept { return layout_.rowHeight + layout_.rowGap; }

    [[nodiscard]] Rect rowRect(std::size_t row) const noexcept;
    [[nodiscard]] Rect swatchRect(std::size_t row) const noexcept;
    [[nodiscard]] Rect nameRect(std::size_t row) const noexcept;
    [[nodiscard]] Rect muteRect(std::size_t row) const noexcept;
    [[nodiscard]] Rect soloRect(std::size_t row) const noexcept;
    [[nodiscard]] Rect volumeRect(std::size_t row) const noexcept;
    [[nodiscard]] Rect stepRect(std::size_t row, int step) const noexcept;

    /// Total height of `channelCount` rows.
    [[nodiscard]] double contentHeight(std::size_t channelCount) const noexcept;

    // ── Hit testing ─────────────────────────────────────────────────────────

    /// What is under a point. `pattern` bounds the step area; a click past the
    /// end of the pattern is not a step.
    [[nodiscard]] Hit hitTest(std::size_t channelCount, const Pattern* pattern,
                              double x, double y) const noexcept;

    /// Fader position 0..1 for a point inside a row's volume control.
    [[nodiscard]] double volumeForX(std::size_t row, double x) const noexcept;

private:
    Layout layout_;
    Tick   stepTicks_ = incdaw::engine::ticksPerQuarterNote / 4;
    int    firstStep_ = 0;
};

} // namespace incdaw::app
