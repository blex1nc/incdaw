#include "app/devices/DeviceUiValue.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace incdaw::app {

namespace {

[[nodiscard]] bool logarithmicUsable(const DeviceUiRange& range) noexcept
{
    return range.skew == DeviceSkew::logarithmic && range.min > 0.0 && range.max > range.min;
}

/// A short `snprintf` into a std::string. Declared with the printf format
/// attribute so every call site is still format-checked — which is also what
/// tells the compiler the format forwarded to `vsnprintf` is trustworthy.
#if defined(__clang__) || defined(__GNUC__)
[[gnu::format(printf, 1, 2)]]
#endif
[[nodiscard]] std::string printf(const char* format, ...);

[[nodiscard]] std::string printf(const char* format, ...)
{
    char buffer[48];

    std::va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);

    return buffer;
}

} // namespace

DeviceUiRange effectiveRange(const DeviceUiWidget& widget,
                             const DeviceUiParameter& parameter) noexcept
{
    if (widget.range.has_value())
        return *widget.range;

    DeviceUiRange range;
    range.min  = parameter.minValue;
    range.max  = parameter.maxValue;
    range.step = parameter.stepped ? 1.0 : 0.0;
    return range;
}

double toNormalised(double value, const DeviceUiRange& range) noexcept
{
    if (range.max <= range.min)
        return 0.0;

    if (logarithmicUsable(range))
        return std::log(std::max(value, range.min) / range.min) / std::log(range.max / range.min);

    return std::clamp((value - range.min) / (range.max - range.min), 0.0, 1.0);
}

double fromNormalised(double normalised, const DeviceUiRange& range) noexcept
{
    const double clamped = std::clamp(normalised, 0.0, 1.0);

    if (logarithmicUsable(range))
        return range.min * std::pow(range.max / range.min, clamped);

    return range.min + (range.max - range.min) * clamped;
}

double constrainValue(double value, const DeviceUiWidget& widget,
                      const DeviceUiParameter& parameter) noexcept
{
    double result = std::clamp(value, parameter.minValue, parameter.maxValue);

    if (parameter.stepped)
        result = std::round(result);
    else if (widget.range.has_value() && widget.range->step > 0.0)
        result = std::round(result / widget.range->step) * widget.range->step;

    if (widget.bipolar && parameter.minValue < 0.0 && parameter.maxValue > 0.0) {
        const double detent = (parameter.maxValue - parameter.minValue) * bipolarDetentFraction;
        if (std::abs(result) < detent)
            result = 0.0;
    }

    return std::clamp(result, parameter.minValue, parameter.maxValue);
}

double resetValue(const DeviceUiWidget& widget, const DeviceUiParameter& parameter) noexcept
{
    (void)widget;
    return std::clamp(parameter.defaultValue, parameter.minValue, parameter.maxValue);
}

std::string formatDeviceValue(double value, const DeviceUiWidget& widget,
                              const DeviceUiParameter& parameter)
{
    if (!widget.choices.empty()) {
        const auto index = static_cast<long>(std::lround(value));
        if (index >= 0 && static_cast<std::size_t>(index) < widget.choices.size())
            return widget.choices[static_cast<std::size_t>(index)];
    }

    if (parameter.stepped)
        return printf("%.0f", std::round(value));

    const std::string& unit = widget.unit;

    if (unit == "dB")
        return widget.bipolar ? (value == 0.0 ? "0.0 dB" : printf("%+.1f dB", value))
                              : printf("%.1f dB", value);

    if (unit == "Hz")
        return value >= 1000.0 ? printf("%.2f kHz", value / 1000.0) : printf("%.0f Hz", value);

    if (unit == "ms")
        return value >= 100.0 ? printf("%.0f ms", value) : printf("%.1f ms", value);

    if (unit == "s")
        return printf("%.2f s", value);

    if (unit == "%")
        return printf("%.0f %%", value);

    if (unit == "st")
        return widget.bipolar ? printf("%+.1f st", value) : printf("%.1f st", value);

    if (unit == "x")
        return printf("%.2fx", value);

    if (!unit.empty())
        return printf("%.3g", value) + " " + unit;

    return printf("%.3g", value);
}

} // namespace incdaw::app
