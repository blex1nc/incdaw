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
        /// One line, the shape a step sequencer's row has had since hardware:
        /// an on/off lamp, the channel's own button, its two knobs, then the
        /// steps. Stacking those onto two lines halves how many channels fit
        /// on screen, and a rack is read by scanning down it.
        double rowHeight   = 32.0;
        double rowGap      = 3.0;

        /// Lamp, button, pan and volume live left of the grid.
        double headerWidth = 250.0;

        /// The band above the first row, where the grid is numbered. Zero
        /// leaves the rows where they have always been, which is what the
        /// geometry tests describe; the view sets a real height.
        double rulerHeight = 0.0;

        double stepWidth   = 26.0;
        double stepGap     = 3.0;

        /// The mute lamp: round, small, first thing in the row.
        double ledWidth    = 13.0;

        /// The solo switch beside it.
        double buttonWidth = 16.0;

        /// Pan and volume are both knobs, and both this size. A knob rather
        /// than a fader because two of them fit where one fader would, and a
        /// rack row needs the width for its steps.
        double knobWidth   = 22.0;

        double padding     = 6.0;

        /// Extra space between groups of four steps.
        ///
        /// Beats are read spatially before they are read by colour: four-four
        /// grouping is what makes a sixteen-step bar countable at a glance,
        /// and it survives a colour scheme that shading does not.
        double stepGroupGap = 7.0;

        /// Steps per group.
        int    stepsPerGroup = 4;
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

    enum class Zone { none, name, mute, solo, volume, pan, step };

    /// Points of vertical drag that sweep a knob end to end.
    ///
    /// Both knobs use it. Turning a 22-point knob by dragging across it would
    /// give a tenth of a point per percent; every DAW turns a knob by dragging
    /// up and down over a distance much larger than the knob, and this is that
    /// distance.
    static constexpr double knobDragTravel = 140.0;

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

    /// The ruler band itself, and the cell above `step` in it. `width` is the
    /// view's, because the band runs to the trailing edge.
    [[nodiscard]] Rect rulerRect(double width) const noexcept;
    [[nodiscard]] Rect rulerStepRect(int step) const noexcept;
    [[nodiscard]] double rowPitch() const noexcept { return layout_.rowHeight + layout_.rowGap; }

    [[nodiscard]] Rect rowRect(std::size_t row) const noexcept;
    [[nodiscard]] Rect swatchRect(std::size_t row) const noexcept;

    /// Where the channel's button begins, clear of the lamp and the switch.
    [[nodiscard]] double contentLeft() const noexcept;
    /// The channel's button: colour, name, and — where there is room — what
    /// makes its sound, set right. Clicking it selects; double-clicking
    /// renames.
    [[nodiscard]] Rect nameRect(std::size_t row) const noexcept;

    [[nodiscard]] Rect muteRect(std::size_t row) const noexcept;
    [[nodiscard]] Rect soloRect(std::size_t row) const noexcept;
    [[nodiscard]] Rect volumeRect(std::size_t row) const noexcept;
    [[nodiscard]] Rect panRect(std::size_t row) const noexcept;
    [[nodiscard]] Rect stepRect(std::size_t row, int step) const noexcept;

    /// A step's horizontal offset from the grid's left edge, group gaps
    /// included. The grid, the ruler and the hit test all measure with this:
    /// the gaps make the offset non-linear in the step index, and three
    /// separate calculations would drift apart.
    [[nodiscard]] double stepOffset(int step) const noexcept;

    /// Total height of `channelCount` rows.
    [[nodiscard]] double contentHeight(std::size_t channelCount) const noexcept;

    // ── Hit testing ─────────────────────────────────────────────────────────

    /// What is under a point. `pattern` bounds the step area; a click past the
    /// end of the pattern is not a step.
    [[nodiscard]] Hit hitTest(std::size_t channelCount, const Pattern* pattern,
                              double x, double y) const noexcept;

    /// A knob's value after a vertical drag: what it was when the drag began,
    /// where the cursor was then, where it is now, and the range to stay
    /// inside. Up increases, which is what a knob turning clockwise under an
    /// upward drag does.
    [[nodiscard]] static double knobForDrag(double startValue, double startY, double y,
                                            double minimum, double maximum) noexcept;

    /// Pan for a vertical drag. Bipolar, so a full sweep crosses -1 to 1.
    [[nodiscard]] static double panForDrag(double startPan, double startY, double y) noexcept
    {
        return knobForDrag(startPan, startY, y, -1.0, 1.0);
    }

    /// Volume for a vertical drag, 0..1.
    [[nodiscard]] static double volumeForDrag(double startVolume, double startY,
                                              double y) noexcept
    {
        return knobForDrag(startVolume, startY, y, 0.0, 1.0);
    }

private:
    Layout layout_;
    Tick   stepTicks_ = incdaw::engine::ticksPerQuarterNote / 4;
    int    firstStep_ = 0;
};

} // namespace incdaw::app
