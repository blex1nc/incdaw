// The declarative face of a device: what its editor shows, as plain data.
//
// A DeviceUiSpec is a tree of layout widgets whose leaves name parameter ids.
// It knows nothing of AppKit and nothing of DSP — `app/devices/*` says "a
// bipolar knob on parameter 3, in dB"; the shell's DevicePanel turns that
// into a view, and the engine turns the written value into gain. That is the
// whole point: once the shell renders specs, adding a device never touches
// the shell (docs/plugin-archive/00-CONTRACTS.md §4, §5).
//
// This header is FROZEN by the contract. Sixty panels are written against it
// in parallel; extend it additively, publish every extension in
// 00-CONTRACTS.md, and never rename or remove a member.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace incdaw::app {

/// The widget vocabulary — exactly the 23 of 00-CONTRACTS.md §4. A surface
/// the vocabulary cannot carry goes through `DeviceUiSpec::customView`.
enum class DeviceWidget : std::uint8_t {
    // ── Controls: `parameters[0]` unless noted ──────────────────────────────
    knob,            ///< rotary control
    slider,          ///< linear control
    faderWall,       ///< N faders side by side; one parameter per fader (harmonic bank, step levels)
    toggle,          ///< on/off; the parameter is 0 or 1
    combo,           ///< stepped choice; `choices` name the steps in value order
    xyPad,           ///< `parameters = {x, y}`; the renderer may record a path
    // ── Drawn, interactive ──────────────────────────────────────────────────
    drawableCurve,   ///< a curve with draggable points; `plot` names what it shows
    envelope,        ///< `parameters = {attack, decay, sustain, release}`; extra points through StateIO
    stepGrid,        ///< `columns` steps × `rows` lanes; cell state through StateIO
    padGrid,         ///< `columns` × `rows` pads; selection and trigger
    keyboard,        ///< a piano keyboard: zone display and note trigger
    zoneMap,         ///< key × velocity zones; zone state through StateIO
    matrix,          ///< `columns` × `rows` routing/modulation matrix; cell state through StateIO
    // ── Displays: read-only, fed by the device's published snapshot ─────────
    scope,           ///< time-domain trace
    spectrum,        ///< magnitude spectrum
    goniometer,      ///< stereo field
    meter,           ///< level(s); `parameters` may name the value(s) shown
    waveform,        ///< a sample/IR overview; `plot` names which
    // ── Layout ──────────────────────────────────────────────────────────────
    label,           ///< static text (`label`)
    section,         ///< titled group; `collapsed` starts it folded
    row,             ///< children left to right
    grid,            ///< children in `columns` columns
    tab,             ///< one page; adjacent `tab` siblings form one tab strip
};

/// How a control's travel maps onto the parameter's range.
enum class DeviceSkew : std::uint8_t {
    linear,
    logarithmic,   ///< equal travel per octave — frequencies, times; needs min > 0
};

/// A display range for one control. Absent, the parameter table's own
/// min/max and `stepped` flag apply; the usual reason to state one is skew.
struct DeviceUiRange {
    double     min  = 0.0;
    double     max  = 1.0;
    DeviceSkew skew = DeviceSkew::linear;
    double     step = 0.0;   ///< 0 = continuous
};

/// One draggable handle on a drawn widget (a curve's band, an envelope's
/// corner, an xy-pad's dot). Each axis names the parameter that axis writes;
/// an axis without one does not move. WHERE the handle sits is arithmetic
/// on the widget's `xAxis`/`yAxis`, so it is the same in the renderer and in
/// a test — see app/devices/DeviceUiPlot.h.
struct DeviceUiPoint {
    std::optional<std::uint32_t> x;   ///< horizontal drag writes this
    std::optional<std::uint32_t> y;   ///< vertical drag writes this
    std::optional<std::uint32_t> z;   ///< the scroll wheel writes this (a band's Q, a point's tension)
    std::string label;                ///< drawn beside the handle ("LOW", "MID", ...)
};

/// One node of the tree.
struct DeviceUiWidget {
    DeviceWidget kind = DeviceWidget::knob;

    std::string label;                        ///< caption; a section's or tab's title; a label's text
    std::vector<std::uint32_t> parameters;    ///< the parameter id(s) this widget reads and writes
    std::optional<DeviceUiRange> range;       ///< display range/skew override
    std::string unit;                         ///< "dB", "Hz", "ms", "%", "st", "x", "" — display only
    std::string tint;                         ///< theme ink token ("accent", "midi", "audio", ...); "" = the panel default
    std::string plot;                         ///< drawn widgets: what the renderer plots ("eq-response", "transfer", "gate", "sample", ...)
    std::vector<std::string> choices;         ///< combo: names of the stepped values, in value order

    std::optional<DeviceUiRange> xAxis;       ///< drawn widgets: the horizontal axis (a curve's frequency); unset = the renderer's own
    std::optional<DeviceUiRange> yAxis;       ///< drawn widgets: the vertical axis (a curve's dB); unset = the renderer's own
    std::vector<DeviceUiPoint>   points;      ///< drawn widgets: the draggable handles, in drawing order

    std::uint16_t columns = 0;                ///< grid, fader-wall, step-grid, pad-grid, matrix
    std::uint16_t rows    = 0;                ///< step-grid, pad-grid, matrix

    bool bipolar   = false;                   ///< knob/slider: zero at centre, ± display, detent at zero
    bool collapsed = false;                   ///< section: starts folded ("Advanced")

    std::vector<DeviceUiWidget> children;     ///< section, row, grid, tab

    // ── Chainable modifiers, so a spec reads as one expression ──────────────
    [[nodiscard]] DeviceUiWidget withRange(DeviceUiRange value) &&
    {
        range = value;
        return std::move(*this);
    }
    [[nodiscard]] DeviceUiWidget withUnit(std::string value) &&
    {
        unit = std::move(value);
        return std::move(*this);
    }
    [[nodiscard]] DeviceUiWidget withTint(std::string value) &&
    {
        tint = std::move(value);
        return std::move(*this);
    }
    [[nodiscard]] DeviceUiWidget withPlot(std::string value) &&
    {
        plot = std::move(value);
        return std::move(*this);
    }
    [[nodiscard]] DeviceUiWidget asBipolar() &&
    {
        bipolar = true;
        return std::move(*this);
    }
    [[nodiscard]] DeviceUiWidget startCollapsed() &&
    {
        collapsed = true;
        return std::move(*this);
    }
    /// The axes a drawn widget is plotted against. Stating them is what makes
    /// its handles land ON the curve: the renderer plots from the same pair.
    [[nodiscard]] DeviceUiWidget withAxes(DeviceUiRange horizontal, DeviceUiRange vertical) &&
    {
        xAxis = horizontal;
        yAxis = vertical;
        return std::move(*this);
    }
    [[nodiscard]] DeviceUiWidget withPoints(std::vector<DeviceUiPoint> value) &&
    {
        points = std::move(value);
        return std::move(*this);
    }
};

/// A device's whole editor.
struct DeviceUiSpec {
    std::string uid;                   ///< "incdaw.tone" — the catalogue key
    std::string title;                 ///< the window title

    double preferredWidth  = 0.0;      ///< points; 0 = fit the content
    double preferredHeight = 0.0;

    std::vector<DeviceUiWidget> root;  ///< top to bottom

    /// Escape hatch: the name of a bespoke Objective-C++ view class the shell
    /// instantiates INSTEAD of rendering `root`. Budget: eight devices across
    /// the archive, each justified in docs/DECISIONS.md (§4).
    std::string customView;
};

// ── Constructors for the common widgets ─────────────────────────────────────
// Plain functions that fill in the struct; nothing here is required, and a
// spec may also be built field by field.

namespace widgets {

[[nodiscard]] inline DeviceUiWidget leaf(DeviceWidget kind, std::uint32_t parameter,
                                         std::string label)
{
    DeviceUiWidget widget;
    widget.kind       = kind;
    widget.parameters = {parameter};
    widget.label      = std::move(label);
    return widget;
}

[[nodiscard]] inline DeviceUiWidget knob(std::uint32_t parameter, std::string label)
{
    return leaf(DeviceWidget::knob, parameter, std::move(label));
}

[[nodiscard]] inline DeviceUiWidget slider(std::uint32_t parameter, std::string label)
{
    return leaf(DeviceWidget::slider, parameter, std::move(label));
}

[[nodiscard]] inline DeviceUiWidget toggle(std::uint32_t parameter, std::string label)
{
    return leaf(DeviceWidget::toggle, parameter, std::move(label));
}

[[nodiscard]] inline DeviceUiWidget combo(std::uint32_t parameter, std::string label,
                                          std::vector<std::string> choices)
{
    DeviceUiWidget widget = leaf(DeviceWidget::combo, parameter, std::move(label));
    widget.choices        = std::move(choices);
    return widget;
}

/// A drawn widget over several parameters (an envelope's four, an xy-pad's
/// two, a fader wall's many); `plot` says what a curve or waveform shows.
[[nodiscard]] inline DeviceUiWidget drawn(DeviceWidget kind, std::vector<std::uint32_t> parameters,
                                          std::string label, std::string plot = {})
{
    DeviceUiWidget widget;
    widget.kind       = kind;
    widget.parameters = std::move(parameters);
    widget.label      = std::move(label);
    widget.plot       = std::move(plot);
    return widget;
}

[[nodiscard]] inline DeviceUiWidget meter(std::string label, std::vector<std::uint32_t> parameters = {})
{
    return drawn(DeviceWidget::meter, std::move(parameters), std::move(label));
}

[[nodiscard]] inline DeviceUiWidget label(std::string text)
{
    DeviceUiWidget widget;
    widget.kind  = DeviceWidget::label;
    widget.label = std::move(text);
    return widget;
}

[[nodiscard]] inline DeviceUiWidget container(DeviceWidget kind, std::string title,
                                              std::vector<DeviceUiWidget> children)
{
    DeviceUiWidget widget;
    widget.kind     = kind;
    widget.label    = std::move(title);
    widget.children = std::move(children);
    return widget;
}

[[nodiscard]] inline DeviceUiWidget section(std::string title, std::vector<DeviceUiWidget> children)
{
    return container(DeviceWidget::section, std::move(title), std::move(children));
}

[[nodiscard]] inline DeviceUiWidget row(std::vector<DeviceUiWidget> children)
{
    return container(DeviceWidget::row, {}, std::move(children));
}

[[nodiscard]] inline DeviceUiWidget grid(std::uint16_t columns, std::vector<DeviceUiWidget> children)
{
    DeviceUiWidget widget = container(DeviceWidget::grid, {}, std::move(children));
    widget.columns        = columns;
    return widget;
}

[[nodiscard]] inline DeviceUiWidget tab(std::string title, std::vector<DeviceUiWidget> children)
{
    return container(DeviceWidget::tab, std::move(title), std::move(children));
}

} // namespace widgets

} // namespace incdaw::app
