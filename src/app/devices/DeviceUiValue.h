// The arithmetic between a control's travel and a parameter's value, and
// the text a readout shows — shared by every renderer of a DeviceUiSpec, so
// that a frequency slider and a dB knob mean the same thing on every panel.

#pragma once

#include "app/devices/DeviceUiSpec.h"

#include <string>

namespace incdaw::app {

/// The parameter as the panel knows it: the device's own table row, plus
/// the live value. Mirrors the shell's row dictionary.
struct DeviceUiParameter {
    std::uint32_t id           = 0;
    double        minValue     = 0.0;
    double        maxValue     = 1.0;
    double        defaultValue = 0.0;
    double        value        = 0.0;
    bool          stepped      = false;
};

/// The display range a widget uses: its own `range` when stated, else the
/// parameter's table range, linear.
[[nodiscard]] DeviceUiRange effectiveRange(const DeviceUiWidget& widget,
                                           const DeviceUiParameter& parameter) noexcept;

/// 0..1 travel for `value` under `range` (log skew needs min > 0; a
/// logarithmic range that cannot be is treated as linear).
[[nodiscard]] double toNormalised(double value, const DeviceUiRange& range) noexcept;
[[nodiscard]] double fromNormalised(double normalised, const DeviceUiRange& range) noexcept;

/// Clamps into the parameter's range, rounds a stepped one, and snaps a
/// bipolar control to exactly zero inside the detent band — so "back to
/// neutral" is a gesture rather than a hunt for the exact pixel.
[[nodiscard]] double constrainValue(double value, const DeviceUiWidget& widget,
                                    const DeviceUiParameter& parameter) noexcept;

/// The value a double-click returns a control to: the table default.
[[nodiscard]] double resetValue(const DeviceUiWidget& widget,
                                const DeviceUiParameter& parameter) noexcept;

/// "+3.5 dB", "1.20 kHz", "250 ms", "Sine" (a combo's choice), "0.707".
[[nodiscard]] std::string formatDeviceValue(double value, const DeviceUiWidget& widget,
                                            const DeviceUiParameter& parameter);

/// The fraction of the range inside which a bipolar control snaps to zero.
inline constexpr double bipolarDetentFraction = 0.025;

} // namespace incdaw::app
