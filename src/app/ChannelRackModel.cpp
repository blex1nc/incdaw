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

int ChannelRackModel::visibleStepCount(double width) const noexcept
{
    const double pitch = layout_.stepWidth + layout_.stepGap;
    if (pitch <= 0.0 || width <= 0.0)
        return 0;

    return static_cast<int>(std::floor(width / pitch));
}

double ChannelRackModel::rowY(std::size_t row) const noexcept
{
    return static_cast<double>(row) * rowPitch();
}

ChannelRackModel::Rect ChannelRackModel::rowRect(std::size_t row) const noexcept
{
    return {0.0, rowY(row), layout_.headerWidth, layout_.rowHeight};
}

ChannelRackModel::Rect ChannelRackModel::swatchRect(std::size_t row) const noexcept
{
    return {0.0, rowY(row), layout_.swatchWidth, layout_.rowHeight};
}

ChannelRackModel::Rect ChannelRackModel::nameRect(std::size_t row) const noexcept
{
    const double left  = layout_.swatchWidth + layout_.padding;
    const double right = muteRect(row).x - layout_.padding;

    return {left, rowY(row), std::max(0.0, right - left), layout_.rowHeight};
}

ChannelRackModel::Rect ChannelRackModel::muteRect(std::size_t row) const noexcept
{
    const double x = layout_.headerWidth - layout_.padding - layout_.volumeWidth
                   - 2.0 * layout_.buttonWidth - 2.0 * layout_.padding;

    // Square and centred in the row: a mute switch that stretches with the row
    // height stops looking like a switch as soon as rows grow.
    const double size  = std::min(layout_.buttonWidth, layout_.rowHeight - 8.0);
    const double inset = (layout_.rowHeight - size) / 2.0;

    return {x, rowY(row) + inset, size, size};
}

ChannelRackModel::Rect ChannelRackModel::soloRect(std::size_t row) const noexcept
{
    const Rect mute = muteRect(row);
    return {mute.x + layout_.buttonWidth + layout_.padding, mute.y, mute.width, mute.height};

}

ChannelRackModel::Rect ChannelRackModel::volumeRect(std::size_t row) const noexcept
{
    const double x = layout_.headerWidth - layout_.padding - layout_.volumeWidth;
    const double height = std::min(14.0, layout_.rowHeight - 12.0);

    return {x, rowY(row) + (layout_.rowHeight - height) / 2.0, layout_.volumeWidth, height};
}

ChannelRackModel::Rect ChannelRackModel::stepRect(std::size_t row, int step) const noexcept
{
    const double pitch = layout_.stepWidth + layout_.stepGap;
    const double x = layout_.headerWidth + static_cast<double>(step - firstStep_) * pitch;

    const double inset = std::min(5.0, layout_.rowHeight / 6.0);
    return {x, rowY(row) + inset, layout_.stepWidth, layout_.rowHeight - 2.0 * inset};
}

double ChannelRackModel::contentHeight(std::size_t channelCount) const noexcept
{
    if (channelCount == 0)
        return 0.0;

    return static_cast<double>(channelCount) * rowPitch() - layout_.rowGap;
}

ChannelRackModel::Hit ChannelRackModel::hitTest(std::size_t channelCount, const Pattern* pattern,
                                                double x, double y) const noexcept
{
    Hit hit;

    if (y < 0.0 || channelCount == 0)
        return hit;

    const double pitch = rowPitch();
    if (pitch <= 0.0)
        return hit;

    const double rowIndex = std::floor(y / pitch);
    if (rowIndex < 0.0 || rowIndex >= static_cast<double>(channelCount))
        return hit;

    const std::size_t row = static_cast<std::size_t>(rowIndex);

    // The gap between rows belongs to neither of them. Treating it as a hit
    // makes a click near an edge act on the wrong channel.
    if (y - rowIndex * pitch >= layout_.rowHeight)
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
        else
            hit.zone = Zone::name;

        return hit;
    }

    const double stepPitch = layout_.stepWidth + layout_.stepGap;
    if (stepPitch <= 0.0)
        return hit;

    const int step = firstStep_
                   + static_cast<int>(std::floor((x - layout_.headerWidth) / stepPitch));

    // Past the end of the pattern there is no step to toggle, and inventing one
    // would silently extend the pattern.
    if (pattern == nullptr || step < 0 || step >= stepCount(*pattern))
        return hit;

    // The gap between cells, like the gap between rows, is not a cell.
    const Rect cell = stepRect(row, step);
    if (x >= cell.x + cell.width)
        return hit;

    hit.zone = Zone::step;
    hit.step = step;
    return hit;
}

double ChannelRackModel::volumeForX(std::size_t row, double x) const noexcept
{
    const Rect rect = volumeRect(row);
    if (rect.width <= 0.0)
        return 0.0;

    return std::clamp((x - rect.x) / rect.width, 0.0, 1.0);
}

} // namespace incdaw::app
