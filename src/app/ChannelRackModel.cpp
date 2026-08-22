#include "app/ChannelRackModel.h"

#include <algorithm>
#include <cmath>

namespace incdaw::app {

void ChannelRackModel::setStepTicks(Tick ticks) noexcept
{
    stepTicks_ = ticks > 0 ? ticks : incdaw::engine::ticksPerQuarterNote / 4;
}

int ChannelRackModel::stepCount(const Pattern& pattern) const noexcept
{
    if (pattern.length <= 0)
        return 0;

    // Round up: a pattern one tick longer than four steps still has a fifth
    // step, and the notes a user put in it must remain reachable.
    const Tick steps = (pattern.length + stepTicks_ - 1) / stepTicks_;
    return static_cast<int>(steps);
}

Tick ChannelRackModel::tickForStep(int step) const noexcept
{
    return static_cast<Tick>(step) * stepTicks_;
}

/// Where a step's cell starts, measured from the first visible step.
///
/// One function so the grid, the ruler and the hit test cannot disagree about
/// where a group gap falls — which they would, since the gap makes the offset
/// non-linear in the step index.
double ChannelRackModel::stepOffset(int step) const noexcept
{
    const int    index = step - firstStep_;
    const double pitch = layout_.stepWidth + layout_.stepGap;

    if (layout_.stepsPerGroup <= 0)
        return static_cast<double>(index) * pitch;

    // Gaps BEFORE this step, counted from the pattern's own step zero rather
    // than from the scroll position: the grouping is a property of the bar, and
    // scrolling must not move where the bars appear to start.
    const auto gapsBefore = [this](int at) {
        return static_cast<double>(at / layout_.stepsPerGroup) * layout_.stepGroupGap;
    };

    return static_cast<double>(index) * pitch + gapsBefore(step) - gapsBefore(firstStep_);
}

int ChannelRackModel::visibleStepCount(double width) const noexcept
{
    const double pitch = layout_.stepWidth + layout_.stepGap;
    if (pitch <= 0.0 || width <= 0.0)
        return 0;

    // Walk rather than divide: the group gaps make the offset non-linear, and
    // dividing would over-report by one cell per group and draw off the edge.
    int step = firstStep_;
    while (stepOffset(step + 1) <= width)
        ++step;

    return step - firstStep_;
}

double ChannelRackModel::rowY(std::size_t row) const noexcept
{
    return layout_.rulerHeight + static_cast<double>(row) * rowPitch();
}

ChannelRackModel::Rect ChannelRackModel::rulerRect(double width) const noexcept
{
    return {0.0, 0.0, std::max(0.0, width), layout_.rulerHeight};
}

ChannelRackModel::Rect ChannelRackModel::rulerStepRect(int step) const noexcept
{
    return {layout_.headerWidth + stepOffset(step), 0.0, layout_.stepWidth,
            layout_.rulerHeight};
}

ChannelRackModel::Rect ChannelRackModel::rowRect(std::size_t row) const noexcept
{
    return {0.0, rowY(row), layout_.headerWidth, layout_.rowHeight};
}

// ── The row, left to right ───────────────────────────────────────────────────
//
// A mute lamp, the channel's own button, its pan and volume knobs, then the
// steps. That order is what a step sequencer's row has looked like since the
// hardware ones, and it survives because it matches how the row is used: the
// lamp is hit constantly while writing a part, the button opens the sound, and
// the knobs are set once and left.

namespace {

constexpr double edgeInset = 4.0;   ///< margin at the row's leading edge

/// Centres a control of `size` in a row that starts at `top`.
double centred(double top, double rowHeight, double size) noexcept
{
    return top + (rowHeight - size) / 2.0;
}

} // namespace

ChannelRackModel::Rect ChannelRackModel::swatchRect(std::size_t row) const noexcept
{
    // The mute lamp IS the colour: it is lit in the channel's own colour when
    // the channel is heard and dark when it is not, so one control carries both
    // the identity and the state. A separate swatch beside it would say the
    // same thing twice and cost the row six points.
    const double size = layout_.ledWidth;
    return {edgeInset, centred(rowY(row), layout_.rowHeight, size), size, size};
}

double ChannelRackModel::contentLeft() const noexcept
{
    return edgeInset + layout_.ledWidth + layout_.padding + layout_.buttonWidth
         + layout_.padding;
}

ChannelRackModel::Rect ChannelRackModel::muteRect(std::size_t row) const noexcept
{
    // The lamp and the mute switch are the same control.
    return swatchRect(row);
}

ChannelRackModel::Rect ChannelRackModel::soloRect(std::size_t row) const noexcept
{
    const double size = layout_.buttonWidth;

    return {edgeInset + layout_.ledWidth + layout_.padding,
            centred(rowY(row), layout_.rowHeight, size), size, size};
}

ChannelRackModel::Rect ChannelRackModel::nameRect(std::size_t row) const noexcept
{
    const double left  = contentLeft();
    const double right = volumeRect(row).x - layout_.padding;

    // Nearly the full row height: the button is the largest target in the row
    // because it is the one that opens the channel's instrument.
    return {left, rowY(row) + 2.0, std::max(0.0, right - left), layout_.rowHeight - 4.0};
}

ChannelRackModel::Rect ChannelRackModel::volumeRect(std::size_t row) const noexcept
{
    const double size = layout_.knobWidth;

    return {panRect(row).x - layout_.padding - size,
            centred(rowY(row), layout_.rowHeight, size), size, size};
}

ChannelRackModel::Rect ChannelRackModel::panRect(std::size_t row) const noexcept
{
    const double size = layout_.knobWidth;

    return {layout_.headerWidth - layout_.padding - size,
            centred(rowY(row), layout_.rowHeight, size), size, size};
}

double ChannelRackModel::knobForDrag(double startValue, double startY, double y,
                                     double minimum, double maximum) noexcept
{
    // Up increases. The travel is the whole range end to end, and much longer
    // than the knob, so the last few percent are reachable without a steady
    // hand.
    const double span = maximum - minimum;
    return std::clamp(startValue - (y - startY) * span / knobDragTravel, minimum, maximum);
}

ChannelRackModel::Rect ChannelRackModel::stepRect(std::size_t row, int step) const noexcept
{
    const double inset = std::min(4.0, layout_.rowHeight / 8.0);

    return {layout_.headerWidth + stepOffset(step), rowY(row) + inset, layout_.stepWidth,
            layout_.rowHeight - 2.0 * inset};
}

double ChannelRackModel::contentHeight(std::size_t channelCount) const noexcept
{
    if (channelCount == 0)
        return layout_.rulerHeight;

    return layout_.rulerHeight + static_cast<double>(channelCount) * rowPitch() - layout_.rowGap;
}

ChannelRackModel::Hit ChannelRackModel::hitTest(std::size_t channelCount, const Pattern* pattern,
                                                double x, double y) const noexcept
{
    Hit hit;

    // The ruler band is a label, not a row: a click in it must not toggle the
    // step underneath it.
    if (y < layout_.rulerHeight || channelCount == 0)
        return hit;

    const double pitch = rowPitch();
    if (pitch <= 0.0)
        return hit;

    const double local    = y - layout_.rulerHeight;
    const double rowIndex = std::floor(local / pitch);
    if (rowIndex < 0.0 || rowIndex >= static_cast<double>(channelCount))
        return hit;

    const std::size_t row = static_cast<std::size_t>(rowIndex);

    // The gap between rows belongs to neither of them. Treating it as a hit
    // makes a click near an edge act on the wrong channel.
    if (local - rowIndex * pitch >= layout_.rowHeight)
        return hit;

    hit.row = row;

    if (x < 0.0)
        return hit;

    if (x < layout_.headerWidth) {
        if (muteRect(row).contains(x, y))
            hit.zone = Zone::mute;
        else if (soloRect(row).contains(x, y))
            hit.zone = Zone::solo;
        else if (volumeRect(row).contains(x, y))
            hit.zone = Zone::volume;
        else if (panRect(row).contains(x, y))
            hit.zone = Zone::pan;
        else
            hit.zone = Zone::name;

        return hit;
    }

    const double stepPitch = layout_.stepWidth + layout_.stepGap;
    if (stepPitch <= 0.0 || pattern == nullptr)
        return hit;

    // An estimate from the constant pitch, then corrected: the group gaps make
    // the true offset non-linear, and an uncorrected divide picks the cell
    // before the right one by a whole step near the end of a long bar.
    const double alongGrid = x - layout_.headerWidth;

    int step = firstStep_ + static_cast<int>(std::floor(alongGrid / stepPitch));

    while (step > firstStep_ && stepOffset(step) > alongGrid)
        --step;
    while (stepOffset(step + 1) <= alongGrid)
        ++step;

    // Past the end of the pattern there is no step to toggle, and inventing one
    // would silently extend the pattern.
    if (step < 0 || step >= stepCount(*pattern))
        return hit;

    // The gap between cells — and the wider gap between groups — is not a cell.
    const Rect cell = stepRect(row, step);
    if (x < cell.x || x >= cell.x + cell.width)
        return hit;

    hit.zone = Zone::step;
    hit.step = step;
    return hit;
}



} // namespace incdaw::app
