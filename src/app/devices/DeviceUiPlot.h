// Where a drawn widget's handles sit, and what dragging one is worth.
//
// A curve, an envelope and an xy-pad all say the same thing: two axes, and
// some points on them that write parameters. That arithmetic is here, with
// no AppKit, for the reason the rest of app/devices is: the renderer and the
// tests must agree to the pixel, and a panel has to be checkable without a
// window server. The renderer plots its curve from the SAME axes this places
// handles on — one source, so a handle never floats off its own curve.
//
// Coordinates are the layout's: top-left, y growing DOWN.

#pragma once

#include "app/devices/DeviceUiLayout.h"
#include "app/devices/DeviceUiSpec.h"
#include "app/devices/DeviceUiValue.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace incdaw::app {

/// The two axes a drawn widget is plotted against: the widget's own when it
/// states them, else `fallback`, which is what the renderer would have used.
struct DeviceUiAxes {
    DeviceUiRange x;
    DeviceUiRange y;
};

[[nodiscard]] DeviceUiAxes plotAxes(const DeviceUiWidget& widget, const DeviceUiAxes& fallback);

/// Where a value sits on one axis inside `plot`, and the value a position on
/// it means. The renderer plots its grid and its curve through exactly these,
/// which is what stops a handle from floating off the line it belongs to.
/// A vertical axis runs from its maximum at the TOP, because layout y grows
/// down; a position outside `plot` clamps to the axis's end rather than
/// running away with the parameter.
[[nodiscard]] double axisPositionX(double value, const DeviceUiRange& range,
                                   const DeviceUiRect& plot) noexcept;
[[nodiscard]] double axisPositionY(double value, const DeviceUiRange& range,
                                   const DeviceUiRect& plot) noexcept;
[[nodiscard]] double axisValueX(double x, const DeviceUiRange& range,
                                const DeviceUiRect& plot) noexcept;
[[nodiscard]] double axisValueY(double y, const DeviceUiRange& range,
                                const DeviceUiRect& plot) noexcept;

/// A handle's centre inside `plot` (the drawing area, already inset from the
/// widget's control rect). An axis the point does not name is centred, which
/// is how a handle that only moves vertically still has somewhere to sit.
[[nodiscard]] DeviceUiRect handleRect(const DeviceUiPoint& point, const DeviceUiAxes& axes,
                                      const DeviceUiRect& plot,
                                      const std::function<double(std::uint32_t)>& valueOf);

/// One parameter write a gesture produces.
struct DeviceUiWrite {
    std::uint32_t id    = 0;
    double        value = 0.0;
};

/// What dragging a handle to (`x`, `y`) inside `plot` writes: one write per
/// axis the point names, each already constrained to its parameter's range.
/// `parameterOf` returns nullptr for an id the device does not carry, and
/// that axis is then simply not written.
[[nodiscard]] std::vector<DeviceUiWrite> handleDrag(
    const DeviceUiPoint& point, const DeviceUiWidget& widget, const DeviceUiAxes& axes,
    const DeviceUiRect& plot, double x, double y,
    const std::function<const DeviceUiParameter*(std::uint32_t)>& parameterOf);

/// What a scroll of `ticks` over a handle writes — its `z` parameter, moved
/// by a fraction of its own travel, so a wheel feels the same on a Q as on a
/// tension. Empty when the point names no `z`.
[[nodiscard]] std::vector<DeviceUiWrite> handleScroll(
    const DeviceUiPoint& point, const DeviceUiWidget& widget, double ticks,
    const std::function<const DeviceUiParameter*(std::uint32_t)>& parameterOf);

/// The index of the handle under (`x`, `y`), or none. Nearest wins when two
/// overlap, so a crowded curve stays usable.
[[nodiscard]] std::optional<std::size_t> handleAt(
    const DeviceUiWidget& widget, const DeviceUiAxes& axes, const DeviceUiRect& plot, double x,
    double y, const std::function<double(std::uint32_t)>& valueOf);

/// Handle metrics, in points — shared by the renderer's drawing and the
/// tests' expectations, as with `layout::`.
namespace plot {

inline constexpr double handleSize  = 11.0;
inline constexpr double grabRadius  = 13.0;   ///< how near the mouse must be
inline constexpr double scrollTravel = 0.04;  ///< of the range, per wheel tick

} // namespace plot

} // namespace incdaw::app
