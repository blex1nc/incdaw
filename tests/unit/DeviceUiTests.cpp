// Plugin archive, Wave 1 — the panel renderer's two halves that hold no
// AppKit (docs/plugin-archive/AGENT-1-FRAMEWORK.md).
//
// ui/macos/DevicePanel.mm draws rectangles it does not compute and writes
// values it does not scale: the geometry is app/devices/DeviceUiLayout.cpp
// and the arithmetic is app/devices/DeviceUiValue.cpp. Both are plain C++,
// so the claims a panel has to keep — it fits its width, nothing overlaps,
// folding a section shortens it, a knob's travel reaches both ends of its
// range and comes back to the same value, a bipolar control can be put back
// to zero by hand — are checked here without a window server. Sixty panels
// will be written against this; it is the surface they all share.

#include "doctest.h"

#include "app/devices/DeviceUiCatalogue.h"
#include "app/devices/DeviceUiLayout.h"
#include "app/devices/DeviceUiPlot.h"
#include "app/devices/DeviceUiSpec.h"
#include "app/devices/DeviceUiValue.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/dsp/effects/ToneEffects.h"

#include <cmath>
#include <optional>
#include <string>
#include <vector>

using namespace incdaw;

namespace {

/// The parameter rows a shell would hand the panel, straight from the
/// device's own table — the same source the audio thread reads.
[[nodiscard]] std::vector<app::DeviceUiParameter> parametersOf(const std::string& uid)
{
    std::vector<app::DeviceUiParameter> rows;

    const engine::dsp::BuiltinEffectInfo* info = engine::dsp::findBuiltinEffect(uid);
    if (info == nullptr)
        return rows;

    for (std::size_t index = 0; index < info->parameterCount; ++index) {
        const engine::dsp::EffectParameter& source = info->parameters[index];

        app::DeviceUiParameter row;
        row.id           = source.id;
        row.minValue     = source.minValue;
        row.maxValue     = source.maxValue;
        row.defaultValue = source.defaultValue;
        row.value        = source.defaultValue;
        row.stepped      = source.stepped;
        rows.push_back(row);
    }

    return rows;
}

[[nodiscard]] const app::DeviceUiParameter* find(
    const std::vector<app::DeviceUiParameter>& rows, std::uint32_t id)
{
    for (const app::DeviceUiParameter& row : rows)
        if (row.id == id)
            return &row;

    return nullptr;
}

} // namespace

// ── The arithmetic ───────────────────────────────────────────────────────────

TEST_CASE("a widget without a range of its own borrows the parameter table's")
{
    app::DeviceUiParameter parameter;
    parameter.minValue = -18.0;
    parameter.maxValue = 18.0;

    const app::DeviceUiWidget plain = app::widgets::knob(0, "Gain");
    const app::DeviceUiRange  range = app::effectiveRange(plain, parameter);
    CHECK(range.min == -18.0);
    CHECK(range.max == 18.0);
    CHECK(range.skew == app::DeviceSkew::linear);

    // A stated range wins — that is how a frequency slider gets a log law
    // out of a table that only knows two numbers.
    const app::DeviceUiRange hz{20.0, 20000.0, app::DeviceSkew::logarithmic, 0.0};
    const app::DeviceUiRange stated =
        app::effectiveRange(app::widgets::slider(0, "Freq").withRange(hz), parameter);
    CHECK(stated.max == 20000.0);
    CHECK(stated.skew == app::DeviceSkew::logarithmic);

    // A stepped parameter carries its step into the borrowed range.
    parameter.stepped = true;
    CHECK(app::effectiveRange(plain, parameter).step == 1.0);
}

TEST_CASE("travel and value are inverses, linear and logarithmic alike")
{
    const app::DeviceUiRange linear{-18.0, 18.0, app::DeviceSkew::linear, 0.0};
    const app::DeviceUiRange hz{20.0, 20000.0, app::DeviceSkew::logarithmic, 0.0};

    for (const app::DeviceUiRange& range : {linear, hz}) {
        CHECK(app::toNormalised(range.min, range) == doctest::Approx(0.0));
        CHECK(app::toNormalised(range.max, range) == doctest::Approx(1.0));
        CHECK(app::fromNormalised(0.0, range) == doctest::Approx(range.min));
        CHECK(app::fromNormalised(1.0, range) == doctest::Approx(range.max));

        for (int step = 0; step <= 20; ++step) {
            const double travel = step / 20.0;
            CHECK(app::toNormalised(app::fromNormalised(travel, range), range)
                  == doctest::Approx(travel).epsilon(1e-9));
        }
    }

    // The log law is what makes a frequency knob usable: half the travel is
    // the geometric middle, not the arithmetic one.
    CHECK(app::fromNormalised(0.5, hz) == doctest::Approx(std::sqrt(20.0 * 20000.0)));
    CHECK(app::fromNormalised(0.5, linear) == doctest::Approx(0.0));

    // Off the ends, and a degenerate range, stay finite rather than NaN.
    CHECK(app::fromNormalised(-3.0, hz) == doctest::Approx(20.0));
    CHECK(app::fromNormalised(4.0, hz) == doctest::Approx(20000.0));
    CHECK(app::toNormalised(1.0, {5.0, 5.0, app::DeviceSkew::linear, 0.0}) == 0.0);

    // A log range that cannot be one (min at or below zero) is treated as
    // linear rather than producing infinities.
    const app::DeviceUiRange bad{0.0, 1.0, app::DeviceSkew::logarithmic, 0.0};
    CHECK(app::fromNormalised(0.5, bad) == doctest::Approx(0.5));
}

TEST_CASE("a drag is clamped, a stepped parameter lands on a step, and zero is a detent")
{
    app::DeviceUiParameter gain;
    gain.minValue     = -18.0;
    gain.maxValue     = 18.0;
    gain.defaultValue = 0.0;

    const app::DeviceUiWidget bipolar = app::widgets::knob(0, "MID").asBipolar();
    const app::DeviceUiWidget plain   = app::widgets::knob(0, "MID");

    CHECK(app::constrainValue(-40.0, plain, gain) == -18.0);
    CHECK(app::constrainValue(40.0, plain, gain) == 18.0);
    CHECK(app::constrainValue(6.0, plain, gain) == doctest::Approx(6.0));

    // Inside the detent band a bipolar control snaps home, so "flat again"
    // is a gesture instead of a hunt for one pixel; outside it, it does not.
    const double detent = (gain.maxValue - gain.minValue) * app::bipolarDetentFraction;
    CHECK(app::constrainValue(detent * 0.5, bipolar, gain) == 0.0);
    CHECK(app::constrainValue(-detent * 0.5, bipolar, gain) == 0.0);
    CHECK(app::constrainValue(detent * 2.0, bipolar, gain) == doctest::Approx(detent * 2.0));

    // A control not declared bipolar never snaps, however near zero it is.
    CHECK(app::constrainValue(detent * 0.5, plain, gain) == doctest::Approx(detent * 0.5));

    // A one-sided range has no zero to snap to even when declared bipolar.
    app::DeviceUiParameter positive;
    positive.minValue = 0.0;
    positive.maxValue = 1.0;
    CHECK(app::constrainValue(0.004, bipolar, positive) == doctest::Approx(0.004));

    // Stepped parameters round; a double-click returns the table default.
    app::DeviceUiParameter mode;
    mode.minValue     = 0.0;
    mode.maxValue     = 3.0;
    mode.defaultValue = 1.0;
    mode.stepped      = true;
    CHECK(app::constrainValue(1.6, plain, mode) == 2.0);
    CHECK(app::constrainValue(9.0, plain, mode) == 3.0);
    CHECK(app::resetValue(plain, mode) == 1.0);
    CHECK(app::resetValue(bipolar, gain) == 0.0);
}

TEST_CASE("a readout says what the control is worth, in the unit the device thinks in")
{
    app::DeviceUiParameter gain;
    gain.minValue = -18.0;
    gain.maxValue = 18.0;

    const app::DeviceUiWidget db = app::widgets::knob(0, "MID").withUnit("dB").asBipolar();
    CHECK(app::formatDeviceValue(3.5, db, gain) == "+3.5 dB");
    CHECK(app::formatDeviceValue(-3.5, db, gain) == "-3.5 dB");
    CHECK(app::formatDeviceValue(0.0, db, gain) == "0.0 dB");

    app::DeviceUiParameter hz;
    hz.minValue = 20.0;
    hz.maxValue = 20000.0;

    const app::DeviceUiWidget freq = app::widgets::slider(0, "Freq").withUnit("Hz");
    CHECK(app::formatDeviceValue(250.0, freq, hz) == "250 Hz");
    CHECK(app::formatDeviceValue(1200.0, freq, hz) == "1.20 kHz");

    const app::DeviceUiWidget ms = app::widgets::slider(0, "Attack").withUnit("ms");
    CHECK(app::formatDeviceValue(12.5, ms, hz) == "12.5 ms");
    CHECK(app::formatDeviceValue(250.0, ms, hz) == "250 ms");

    // A combo reads out its own word, not its index.
    app::DeviceUiParameter shape;
    shape.minValue = 0.0;
    shape.maxValue = 2.0;
    shape.stepped  = true;

    const app::DeviceUiWidget combo =
        app::widgets::combo(0, "Shape", {"Sine", "Saw", "Square"});
    CHECK(app::formatDeviceValue(1.0, combo, shape) == "Saw");
    CHECK(app::formatDeviceValue(9.0, combo, shape) == "9");   // out of range: the number

    // A unitless control still reads out, and an unknown unit is appended.
    const app::DeviceUiWidget q = app::widgets::slider(0, "Q");
    CHECK(app::formatDeviceValue(0.707, q, hz) == "0.707");
    CHECK(app::formatDeviceValue(2.0, app::widgets::slider(0, "Ratio").withUnit("x"), hz)
          == "2.00x");
}

// ── The geometry ─────────────────────────────────────────────────────────────

TEST_CASE("the Tone panel lays out inside its width, with nothing on top of anything")
{
    const app::DeviceUiSpec* spec = app::deviceUiSpec("incdaw.tone");
    REQUIRE(spec != nullptr);

    const app::DeviceUiLayout open = app::layoutDeviceUi(*spec);
    CHECK(open.width == spec->preferredWidth);
    CHECK(open.height > 0.0);

    // Every placement is inside the panel, with the margin honoured.
    for (const app::DeviceUiPlacement& item : open.items) {
        REQUIRE(item.widget != nullptr);
        CHECK(item.frame.x >= app::layout::margin - 0.001);
        CHECK(item.frame.right() <= open.width - app::layout::margin + 0.001);
        CHECK(item.frame.width > 0.0);

        // A folded section's children are placed but hidden, and only what
        // is visible has to fit: the panel is as tall as what it shows.
        if (item.visible)
            CHECK(item.frame.bottom() <= open.height - app::layout::margin + 0.001);

        CHECK(item.frame.height > 0.0);

        // A control that takes the mouse is inside the cell it belongs to.
        if (item.control.width > 0.0 && !app::isContainerWidget(item.widget->kind)) {
            CHECK(item.control.x >= item.frame.x - 0.001);
            CHECK(item.control.right() <= item.frame.right() + 0.001);
        }
    }

    // No two leaves overlap: a click means one control.
    std::vector<const app::DeviceUiPlacement*> leaves;
    for (const app::DeviceUiPlacement& item : open.items)
        if (!app::isContainerWidget(item.widget->kind) && item.visible)
            leaves.push_back(&item);

    REQUIRE(leaves.size() >= 4);
    for (std::size_t a = 0; a < leaves.size(); ++a)
        for (std::size_t b = a + 1; b < leaves.size(); ++b)
            CHECK_FALSE(leaves[a]->frame.overlaps(leaves[b]->frame));

    // The three gain knobs sit side by side on one line, in spec order.
    std::vector<const app::DeviceUiPlacement*> knobs;
    for (const app::DeviceUiPlacement* leaf : leaves)
        if (leaf->widget->kind == app::DeviceWidget::knob)
            knobs.push_back(leaf);

    REQUIRE(knobs.size() == 3);
    CHECK(knobs[0]->frame.y == doctest::Approx(knobs[1]->frame.y));
    CHECK(knobs[1]->frame.y == doctest::Approx(knobs[2]->frame.y));
    CHECK(knobs[0]->frame.x < knobs[1]->frame.x);
    CHECK(knobs[1]->frame.x < knobs[2]->frame.x);
    CHECK(knobs[0]->control.width == doctest::Approx(app::layout::knobSize));
}

TEST_CASE("Advanced opens closed, and unfolding it makes the panel taller, not wider")
{
    const app::DeviceUiSpec* spec = app::deviceUiSpec("incdaw.tone");
    REQUIRE(spec != nullptr);

    const app::DeviceUiLayout folded = app::layoutDeviceUi(*spec);   // the spec's own state

    app::DeviceUiLayoutOptions unfoldAll;
    unfoldAll.isCollapsed = [](const app::DeviceUiWidget&) { return false; };
    const app::DeviceUiLayout unfolded = app::layoutDeviceUi(*spec, unfoldAll);

    CHECK(unfolded.width == folded.width);
    CHECK(unfolded.height > folded.height);

    // Folded, the section's children are placed but hidden — the tree stays
    // whole, so the renderer can unfold without relaying out from scratch.
    CHECK(folded.items.size() == unfolded.items.size());

    const auto visibleSliders = [](const app::DeviceUiLayout& layout) {
        int count = 0;
        for (const app::DeviceUiPlacement& item : layout.items)
            if (item.visible && item.widget->kind == app::DeviceWidget::slider)
                ++count;
        return count;
    };

    CHECK(visibleSliders(folded) == 0);
    CHECK(visibleSliders(unfolded) == 4);

    // A folded section contributes only its header, and nothing below it
    // moves — the knobs above are where they were.
    for (const app::DeviceUiPlacement& item : folded.items)
        if (item.widget->kind == app::DeviceWidget::section)
            CHECK(item.frame.height == doctest::Approx(app::layout::sectionHeader));
}

TEST_CASE("a widget the renderer cannot draw yet still gets a cell, so a spec may ship early")
{
    using namespace app::widgets;

    // Agents 2 and 3 write specs against the whole vocabulary; Wave 2 draws
    // the rest. Until then a panel must open, not fall back.
    app::DeviceUiSpec spec;
    spec.uid   = "incdaw.test.ahead";
    spec.title = "Ahead";
    spec.root  = {
        knob(0, "Drive"),
        drawn(app::DeviceWidget::goniometer, {}, "Stereo", ""),
    };

    CHECK(app::isRenderedWidget(app::DeviceWidget::knob));
    CHECK_FALSE(app::isRenderedWidget(app::DeviceWidget::goniometer));

    const app::DeviceUiLayout layout = app::layoutDeviceUi(spec);
    CHECK(layout.width == app::layout::defaultWidth);
    REQUIRE(layout.items.size() == 2);
    CHECK(layout.items[1].frame.height == doctest::Approx(app::layout::placeholderHeight));
    CHECK_FALSE(layout.items[0].frame.overlaps(layout.items[1].frame));

    // An empty spec is a panel with nothing in it, not a crash.
    app::DeviceUiSpec empty;
    const app::DeviceUiLayout none = app::layoutDeviceUi(empty);
    CHECK(none.items.empty());
    CHECK(none.height == doctest::Approx(app::layout::margin * 2.0));
}

TEST_CASE("a grid is columns of one width on one baseline, and it wraps")
{
    using namespace app::widgets;

    // The Tone panel's three knobs fill their row exactly; a fourth would
    // wrap. Drum machines and modulation banks are written as grids, so the
    // wrap is a claim, not an accident.
    app::DeviceUiSpec spec;
    spec.uid            = "incdaw.test.grid";
    spec.preferredWidth = 420.0;
    spec.root = {grid(3, {knob(0, "A"), knob(1, "B"), knob(2, "C"), knob(3, "D")})};

    const app::DeviceUiLayout layout = app::layoutDeviceUi(spec);

    std::vector<const app::DeviceUiPlacement*> knobs;
    for (const app::DeviceUiPlacement& item : layout.items)
        if (item.widget->kind == app::DeviceWidget::knob)
            knobs.push_back(&item);

    REQUIRE(knobs.size() == 4);

    for (std::size_t index = 1; index < 3; ++index) {
        CHECK(knobs[index]->frame.y == doctest::Approx(knobs[0]->frame.y));
        CHECK(knobs[index]->frame.width == doctest::Approx(knobs[0]->frame.width));
        CHECK(knobs[index]->frame.x > knobs[index - 1]->frame.x);
    }

    // The fourth starts the next line, under the first.
    CHECK(knobs[3]->frame.x == doctest::Approx(knobs[0]->frame.x));
    CHECK(knobs[3]->frame.y > knobs[0]->frame.bottom());

    // And the grid is as tall as the two lines it drew.
    CHECK(layout.height >= knobs[3]->frame.bottom() + app::layout::margin - 0.001);
}

TEST_CASE("a narrower window reflows the panel rather than clipping it")
{
    const app::DeviceUiSpec* spec = app::deviceUiSpec("incdaw.tone");
    REQUIRE(spec != nullptr);

    app::DeviceUiLayoutOptions narrow;
    narrow.width = 320.0;

    const app::DeviceUiLayout layout = app::layoutDeviceUi(*spec, narrow);
    CHECK(layout.width == 320.0);

    for (const app::DeviceUiPlacement& item : layout.items) {
        CHECK(item.frame.right() <= 320.0 - app::layout::margin + 0.001);
        CHECK(item.frame.width > 0.0);
    }
}

// ── The two halves together ──────────────────────────────────────────────────

TEST_CASE("every control of the Tone panel drives a real parameter, end to end")
{
    const app::DeviceUiSpec* spec = app::deviceUiSpec("incdaw.tone");
    REQUIRE(spec != nullptr);

    const std::vector<app::DeviceUiParameter> rows = parametersOf("incdaw.tone");
    REQUIRE_FALSE(rows.empty());

    const app::DeviceUiLayout layout = app::layoutDeviceUi(*spec);

    int driven = 0;
    for (const app::DeviceUiPlacement& item : layout.items) {
        const app::DeviceUiWidget& widget = *item.widget;
        if (widget.parameters.empty() || app::isContainerWidget(widget.kind))
            continue;

        const app::DeviceUiParameter* parameter = find(rows, widget.parameters.front());
        REQUIRE(parameter != nullptr);   // a spec naming an id the device lacks
        ++driven;

        // Dragging the control from end to end stays inside the table's
        // range, and the readout never comes back empty.
        const app::DeviceUiRange range = app::effectiveRange(widget, *parameter);
        for (int step = 0; step <= 10; ++step) {
            const double plain =
                app::constrainValue(app::fromNormalised(step / 10.0, range), widget, *parameter);
            CHECK(plain >= parameter->minValue - 1e-9);
            CHECK(plain <= parameter->maxValue + 1e-9);
            CHECK_FALSE(app::formatDeviceValue(plain, widget, *parameter).empty());
        }
    }

    CHECK(driven == 8);   // the curve, three knobs, four Advanced sliders
}

// ── Drawn widgets: the handles on a plot ─────────────────────────────────────

namespace {

/// The Tone curve, its axes and its plot rectangle — the three things every
/// handle gesture is measured against.
struct CurveUnderTest {
    const app::DeviceUiWidget*          widget = nullptr;
    app::DeviceUiAxes                   axes;
    app::DeviceUiRect                   plot{0.0, 0.0, 400.0, 150.0};
    std::vector<app::DeviceUiParameter> rows;
};

[[nodiscard]] CurveUnderTest toneCurve()
{
    CurveUnderTest out;

    const app::DeviceUiSpec* spec = app::deviceUiSpec("incdaw.tone");
    REQUIRE(spec != nullptr);
    REQUIRE_FALSE(spec->root.empty());

    out.widget = &spec->root[0];
    REQUIRE(out.widget->kind == app::DeviceWidget::drawableCurve);

    // The renderer's own defaults, which the spec is expected to override.
    app::DeviceUiAxes fallback;
    fallback.x = {20.0, 20000.0, app::DeviceSkew::logarithmic, 0.0};
    fallback.y = {-24.0, 24.0, app::DeviceSkew::linear, 0.0};

    out.axes = app::plotAxes(*out.widget, fallback);
    out.rows = parametersOf("incdaw.tone");
    REQUIRE_FALSE(out.rows.empty());

    return out;
}

} // namespace

TEST_CASE("the curve names its own axes, and its handles sit on the values they carry")
{
    const CurveUnderTest curve = toneCurve();
    using Eq = engine::dsp::EqEffect;

    // The spec states ±18 dB rather than taking the renderer's ±24: stating
    // axes is what makes a handle land on the line.
    CHECK(curve.axes.x.skew == app::DeviceSkew::logarithmic);
    CHECK(curve.axes.y.max == doctest::Approx(18.0));
    CHECK(curve.axes.y.min == doctest::Approx(-18.0));

    REQUIRE(curve.widget->points.size() == 3);
    CHECK(curve.widget->points[0].label == "LOW");
    CHECK(curve.widget->points[1].label == "MID");
    CHECK(curve.widget->points[2].label == "HIGH");

    // Only the peak band has a width to reach; a shelf has none.
    CHECK(curve.widget->points[1].z == std::optional<std::uint32_t>{Eq::midQ});
    CHECK_FALSE(curve.widget->points[0].z.has_value());
    CHECK_FALSE(curve.widget->points[2].z.has_value());

    // Every id a handle names is one the effect's own table declares.
    for (const app::DeviceUiPoint& point : curve.widget->points)
        for (const std::optional<std::uint32_t>& id : {point.x, point.y, point.z})
            if (id.has_value())
                CHECK(find(curve.rows, *id) != nullptr);

    const auto valueOf = [&](std::uint32_t id) {
        const app::DeviceUiParameter* row = find(curve.rows, id);
        return row != nullptr ? row->value : 0.0;
    };

    // The MID handle is where its frequency and its gain say it is.
    const app::DeviceUiRect mid =
        app::handleRect(curve.widget->points[1], curve.axes, curve.plot, valueOf);

    CHECK(mid.x + mid.width / 2.0
          == doctest::Approx(app::axisPositionX(valueOf(Eq::midFreq), curve.axes.x, curve.plot)));
    CHECK(mid.y + mid.height / 2.0
          == doctest::Approx(app::axisPositionY(valueOf(Eq::midGainDb), curve.axes.y,
                                                curve.plot)));
    CHECK(mid.width == doctest::Approx(app::plot::handleSize));

    // A gain of 0 dB is the middle of a symmetric axis, and the axis is
    // inverted: more dB is FURTHER UP, which is a smaller layout y.
    CHECK(app::axisPositionY(0.0, curve.axes.y, curve.plot)
          == doctest::Approx(curve.plot.y + curve.plot.height / 2.0));
    CHECK(app::axisPositionY(12.0, curve.axes.y, curve.plot)
          < app::axisPositionY(-12.0, curve.axes.y, curve.plot));

    // An axis a point does not name has nowhere to be but the middle.
    app::DeviceUiPoint vertical;
    vertical.y = Eq::midGainDb;
    const app::DeviceUiRect only =
        app::handleRect(vertical, curve.axes, curve.plot, valueOf);
    CHECK(only.x + only.width / 2.0 == doctest::Approx(curve.plot.x + curve.plot.width / 2.0));
}

TEST_CASE("dragging a band writes its frequency and its gain, both inside the table")
{
    const CurveUnderTest curve = toneCurve();
    using Eq = engine::dsp::EqEffect;

    const auto parameterOf = [&](std::uint32_t id) { return find(curve.rows, id); };

    // Drag the MID handle a quarter along and a quarter up.
    const double x = curve.plot.x + curve.plot.width * 0.25;
    const double y = curve.plot.y + curve.plot.height * 0.25;

    const std::vector<app::DeviceUiWrite> writes = app::handleDrag(
        curve.widget->points[1], *curve.widget, curve.axes, curve.plot, x, y, parameterOf);

    REQUIRE(writes.size() == 2);
    CHECK(writes[0].id == Eq::midFreq);
    CHECK(writes[1].id == Eq::midGainDb);

    CHECK(writes[0].value == doctest::Approx(app::axisValueX(x, curve.axes.x, curve.plot)));
    CHECK(writes[1].value > 0.0);   // a quarter DOWN the rect is above the zero line

    for (const app::DeviceUiWrite& write : writes) {
        const app::DeviceUiParameter* row = parameterOf(write.id);
        REQUIRE(row != nullptr);
        CHECK(write.value >= row->minValue);
        CHECK(write.value <= row->maxValue);
    }

    // A drag that leaves the plot pins the band at the end of its axis
    // rather than running away with the parameter.
    const std::vector<app::DeviceUiWrite> escaped =
        app::handleDrag(curve.widget->points[1], *curve.widget, curve.axes, curve.plot,
                        curve.plot.x - 500.0, curve.plot.y - 500.0, parameterOf);

    REQUIRE(escaped.size() == 2);
    CHECK(escaped[0].value == doctest::Approx(std::max(curve.axes.x.min,
                                                       parameterOf(Eq::midFreq)->minValue)));
    CHECK(escaped[1].value == doctest::Approx(std::min(curve.axes.y.max,
                                                       parameterOf(Eq::midGainDb)->maxValue)));

    // An axis the device does not carry is simply not written — a spec that
    // outran its device drags nothing rather than writing the wrong id.
    const std::vector<app::DeviceUiWrite> none =
        app::handleDrag(curve.widget->points[1], *curve.widget, curve.axes, curve.plot, x, y,
                        [](std::uint32_t) -> const app::DeviceUiParameter* { return nullptr; });
    CHECK(none.empty());
}

TEST_CASE("the wheel widens the band under it, and leaves a shelf alone")
{
    const CurveUnderTest curve = toneCurve();
    using Eq = engine::dsp::EqEffect;

    const auto parameterOf = [&](std::uint32_t id) { return find(curve.rows, id); };
    const app::DeviceUiParameter* q = parameterOf(Eq::midQ);
    REQUIRE(q != nullptr);

    const std::vector<app::DeviceUiWrite> up =
        app::handleScroll(curve.widget->points[1], *curve.widget, 1.0, parameterOf);

    REQUIRE(up.size() == 1);
    CHECK(up[0].id == Eq::midQ);
    CHECK(up[0].value > q->value);

    // One tick is one tick of the parameter's OWN travel, whatever it is.
    app::DeviceUiRange range;
    range.min = q->minValue;
    range.max = q->maxValue;
    CHECK(app::toNormalised(up[0].value, range)
          == doctest::Approx(app::toNormalised(q->value, range) + app::plot::scrollTravel));

    const std::vector<app::DeviceUiWrite> down =
        app::handleScroll(curve.widget->points[1], *curve.widget, -1.0, parameterOf);
    REQUIRE(down.size() == 1);
    CHECK(down[0].value < q->value);

    // A shelf has no width to change, and a wheel that did not turn writes
    // nothing at all — the panel passes the scroll on to its scroll view.
    CHECK(app::handleScroll(curve.widget->points[0], *curve.widget, 1.0, parameterOf).empty());
    CHECK(app::handleScroll(curve.widget->points[1], *curve.widget, 0.0, parameterOf).empty());
}

TEST_CASE("the nearest handle wins, and a click away from all of them grabs none")
{
    const CurveUnderTest curve = toneCurve();

    const auto valueOf = [&](std::uint32_t id) {
        const app::DeviceUiParameter* row = find(curve.rows, id);
        return row != nullptr ? row->value : 0.0;
    };

    for (std::size_t index = 0; index < curve.widget->points.size(); ++index) {
        const app::DeviceUiRect cell =
            app::handleRect(curve.widget->points[index], curve.axes, curve.plot, valueOf);

        const std::optional<std::size_t> hit =
            app::handleAt(*curve.widget, curve.axes, curve.plot, cell.x + cell.width / 2.0,
                          cell.y + cell.height / 2.0, valueOf);

        REQUIRE(hit.has_value());
        CHECK(*hit == index);
    }

    // Far from every band: the curve is a picture there, and the click must
    // not jump the nearest band to the pointer.
    const app::DeviceUiRect low =
        app::handleRect(curve.widget->points[0], curve.axes, curve.plot, valueOf);

    CHECK_FALSE(app::handleAt(*curve.widget, curve.axes, curve.plot,
                              low.x + low.width / 2.0 + app::plot::grabRadius * 3.0,
                              low.y + low.height / 2.0 + app::plot::grabRadius * 3.0, valueOf)
                    .has_value());

    // A widget with no points has nothing to grab, at any position.
    app::DeviceUiWidget bare;
    bare.kind = app::DeviceWidget::drawableCurve;
    CHECK_FALSE(app::handleAt(bare, curve.axes, curve.plot, curve.plot.x, curve.plot.y, valueOf)
                    .has_value());
}
