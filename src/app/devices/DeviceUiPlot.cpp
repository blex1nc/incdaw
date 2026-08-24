#include "app/devices/DeviceUiPlot.h"

#include <algorithm>
#include <cmath>

namespace incdaw::app {

/// The travel a value sits at along an axis, with y inverted: a curve's
/// vertical axis runs from its maximum at the TOP to its minimum at the
/// bottom, while layout y grows down.
double axisPositionX(double value, const DeviceUiRange& range, const DeviceUiRect& plot) noexcept
{
    return plot.x + plot.width * toNormalised(value, range);
}

double axisPositionY(double value, const DeviceUiRange& range, const DeviceUiRect& plot) noexcept
{
    return plot.y + plot.height * (1.0 - toNormalised(value, range));
}

double axisValueX(double x, const DeviceUiRange& range, const DeviceUiRect& plot) noexcept
{
    if (plot.width <= 0.0)
        return range.min;

    return fromNormalised((x - plot.x) / plot.width, range);
}

double axisValueY(double y, const DeviceUiRange& range, const DeviceUiRect& plot) noexcept
{
    if (plot.height <= 0.0)
        return range.min;

    return fromNormalised(1.0 - (y - plot.y) / plot.height, range);
}

DeviceUiAxes plotAxes(const DeviceUiWidget& widget, const DeviceUiAxes& fallback)
{
    DeviceUiAxes axes = fallback;

    if (widget.xAxis.has_value())
        axes.x = *widget.xAxis;
    if (widget.yAxis.has_value())
        axes.y = *widget.yAxis;

    return axes;
}

DeviceUiRect handleRect(const DeviceUiPoint& point, const DeviceUiAxes& axes,
                        const DeviceUiRect& plot,
                        const std::function<double(std::uint32_t)>& valueOf)
{
    // An axis the point does not name has nowhere to be but the middle.
    const double centreX =
        point.x.has_value() ? axisPositionX(valueOf(*point.x), axes.x, plot) : plot.x + plot.width / 2.0;
    const double centreY =
        point.y.has_value() ? axisPositionY(valueOf(*point.y), axes.y, plot) : plot.y + plot.height / 2.0;

    return {centreX - plot::handleSize / 2.0, centreY - plot::handleSize / 2.0, plot::handleSize,
            plot::handleSize};
}

std::vector<DeviceUiWrite> handleDrag(
    const DeviceUiPoint& point, const DeviceUiWidget& widget, const DeviceUiAxes& axes,
    const DeviceUiRect& plot, double x, double y,
    const std::function<const DeviceUiParameter*(std::uint32_t)>& parameterOf)
{
    std::vector<DeviceUiWrite> writes;

    if (point.x.has_value())
        if (const DeviceUiParameter* parameter = parameterOf(*point.x))
            writes.push_back(
                {*point.x, constrainValue(axisValueX(x, axes.x, plot), widget, *parameter)});

    if (point.y.has_value())
        if (const DeviceUiParameter* parameter = parameterOf(*point.y))
            writes.push_back(
                {*point.y, constrainValue(axisValueY(y, axes.y, plot), widget, *parameter)});

    return writes;
}

std::vector<DeviceUiWrite> handleScroll(
    const DeviceUiPoint& point, const DeviceUiWidget& widget, double ticks,
    const std::function<const DeviceUiParameter*(std::uint32_t)>& parameterOf)
{
    if (!point.z.has_value() || ticks == 0.0)
        return {};

    const DeviceUiParameter* parameter = parameterOf(*point.z);
    if (parameter == nullptr)
        return {};

    // The wheel moves the parameter along its OWN travel, so one tick is
    // the same gesture whether it is a Q or a tension.
    DeviceUiRange range;
    range.min = parameter->minValue;
    range.max = parameter->maxValue;

    const double moved = toNormalised(parameter->value, range) + ticks * plot::scrollTravel;

    return {{*point.z, constrainValue(fromNormalised(moved, range), widget, *parameter)}};
}

std::optional<std::size_t> handleAt(const DeviceUiWidget& widget, const DeviceUiAxes& axes,
                                    const DeviceUiRect& plot, double x, double y,
                                    const std::function<double(std::uint32_t)>& valueOf)
{
    std::optional<std::size_t> nearest;
    double best = plot::grabRadius * plot::grabRadius;

    for (std::size_t index = 0; index < widget.points.size(); ++index) {
        const DeviceUiRect rect = handleRect(widget.points[index], axes, plot, valueOf);

        const double dx       = x - (rect.x + rect.width / 2.0);
        const double dy       = y - (rect.y + rect.height / 2.0);
        const double distance = dx * dx + dy * dy;

        if (distance <= best) {
            best    = distance;
            nearest = index;
        }
    }

    return nearest;
}

} // namespace incdaw::app
