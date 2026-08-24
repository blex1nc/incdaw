#import "ui/macos/DevicePanel.h"

#include "app/devices/DeviceUiLayout.h"
#include "app/devices/DeviceUiPlot.h"
#include "app/devices/DeviceUiValue.h"
#include "engine/dsp/effects/ToneEffects.h"
#include "ui/ThemePalette.h"
#include "ui/macos/Theme.h"

#import <objc/runtime.h>

#include <algorithm>
#include <cmath>
#include <set>
#include <unordered_map>

namespace theme = incdaw::ui::theme;
namespace app   = incdaw::app;
namespace dsp   = incdaw::engine::dsp;

using incdaw::ui::theme::Ink;

namespace {

/// Taller panels scroll rather than growing past the screen.
constexpr CGFloat visibleLimit = 720.0;

/// Pixels of vertical travel for a knob's full range. Mapping the knob's own
/// height would make every pixel worth too much; ⌥ divides it again.
constexpr double knobTravel = 160.0;
constexpr double fineFactor = 0.25;

/// Response-curve plot bounds.
constexpr double displayDb = 24.0;
constexpr double minPlotHz = 20.0;
constexpr double maxPlotHz = 20000.0;

const void* panelOwnerKey = &panelOwnerKey;

enum class Drag { none, knob, slider, fader, handle };

NSColor* inkForToken(const std::string& token, Ink fallback)
{
    Ink ink;
    if (!token.empty() && theme::inkFromKey(token, ink))
        return theme::ink(ink);

    return theme::ink(fallback);
}

NSString* text(const std::string& value)
{
    return [NSString stringWithUTF8String:value.c_str()];
}

} // namespace

// ── The view ─────────────────────────────────────────────────────────────────

@interface INCDAWDeviceView : NSView
@end

@implementation INCDAWDeviceView {
@public
    const app::DeviceUiSpec*                                  _spec;
    std::unordered_map<std::uint32_t, app::DeviceUiParameter> _params;
    std::set<const app::DeviceUiWidget*>                      _expanded;   ///< sections toggled open
    std::set<const app::DeviceUiWidget*>                      _folded;     ///< sections toggled shut
    app::DeviceUiLayout                                       _layout;
    double                                                    _sampleRate;
    void (^_onWrite)(std::uint32_t, double);

    Drag          _drag;
    std::size_t   _dragItem;
    std::size_t   _dragFader;
    std::size_t   _dragHandle;   ///< which of the widget's points is held
    std::uint32_t _dragParameter;
    double        _dragStartValue;
    NSPoint       _dragStartPoint;

    std::uint32_t _menuParameter;   ///< the combo a popped-up menu writes to
}

- (instancetype)initWithFrame:(NSRect)frame
{
    if ((self = [super initWithFrame:frame]) != nil) {
        _spec       = nullptr;
        _sampleRate = 48000.0;
        _drag       = Drag::none;
    }

    return self;
}

// ── Layout ───────────────────────────────────────────────────────────────────

- (BOOL)isSectionCollapsed:(const app::DeviceUiWidget&)section
{
    if (_expanded.count(&section) != 0)
        return NO;
    if (_folded.count(&section) != 0)
        return YES;
    return section.collapsed;
}

- (void)relayout
{
    app::DeviceUiLayoutOptions options;
    options.isCollapsed = [self](const app::DeviceUiWidget& widget) {
        return [self isSectionCollapsed:widget] == YES;
    };

    _layout = app::layoutDeviceUi(*_spec, options);
}

/// Layout space is top-left, y down; the view is AppKit's, y up. Converting
/// at the edge keeps the theme's primitives in their natural orientation.
- (NSRect)rectFor:(const app::DeviceUiRect&)rect
{
    return NSMakeRect(rect.x, self.bounds.size.height - rect.y - rect.height, rect.width,
                      rect.height);
}

- (app::DeviceUiParameter*)parameterFor:(std::uint32_t)id
{
    const auto found = _params.find(id);
    return found == _params.end() ? nullptr : &found->second;
}

- (const app::DeviceUiParameter*)firstParameterOf:(const app::DeviceUiWidget&)widget
{
    return widget.parameters.empty() ? nullptr : [self parameterFor:widget.parameters.front()];
}

/// A layout y as a view y: the flip `rectFor:` makes, for one coordinate.
- (CGFloat)viewYFor:(double)y
{
    return self.bounds.size.height - static_cast<CGFloat>(y);
}

/// The drawing area inside a drawn widget's cell — the well's own padding
/// taken off. Everything about a plot (its grid, its curve, its handles and
/// the mouse) is measured in here.
- (app::DeviceUiRect)plotRectOf:(const app::DeviceUiPlacement&)item
{
    app::DeviceUiRect rect = item.control;
    rect.x += 6.0;
    rect.y += 6.0;
    rect.width  = std::max(0.0, rect.width - 12.0);
    rect.height = std::max(0.0, rect.height - 12.0);
    return rect;
}

/// The axes a drawn widget is plotted against: its own where it states them,
/// else the renderer's response-curve defaults.
- (app::DeviceUiAxes)axesOf:(const app::DeviceUiWidget&)widget
{
    app::DeviceUiAxes fallback;
    fallback.x = {minPlotHz, maxPlotHz, app::DeviceSkew::logarithmic, 0.0};
    fallback.y = {-displayDb, displayDb, app::DeviceSkew::linear, 0.0};

    return app::plotAxes(widget, fallback);
}

/// The live value of a parameter, for the placing arithmetic. An id the
/// device does not carry reads as zero; the panel refuses such a spec at
/// build time, so this is a floor, not a policy.
- (std::function<double(std::uint32_t)>)valueLookup
{
    return [self](std::uint32_t id) -> double {
        const app::DeviceUiParameter* parameter = [self parameterFor:id];
        return parameter != nullptr ? parameter->value : 0.0;
    };
}

- (std::function<const app::DeviceUiParameter*(std::uint32_t)>)parameterLookup
{
    return [self](std::uint32_t id) -> const app::DeviceUiParameter* {
        return [self parameterFor:id];
    };
}

/// Applies what a gesture produced. One write per axis, each already
/// constrained by app/devices/DeviceUiPlot.cpp.
- (void)apply:(const std::vector<app::DeviceUiWrite>&)writes
       widget:(const app::DeviceUiWidget&)widget
{
    for (const app::DeviceUiWrite& write : writes)
        [self write:write.id value:write.value widget:widget];
}

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;
    theme::fillRect(self.bounds, theme::ink(Ink::panel));

    for (const app::DeviceUiPlacement& item : _layout.items) {
        if (!item.visible)
            continue;

        switch (item.widget->kind) {
        case app::DeviceWidget::section:       [self drawSection:item]; break;
        case app::DeviceWidget::tab:           [self drawTabHeader:item]; break;
        case app::DeviceWidget::row:
        case app::DeviceWidget::grid:          break;
        case app::DeviceWidget::label:         [self drawLabel:item]; break;
        case app::DeviceWidget::knob:          [self drawKnob:item]; break;
        case app::DeviceWidget::slider:        [self drawSlider:item]; break;
        case app::DeviceWidget::toggle:        [self drawToggle:item]; break;
        case app::DeviceWidget::combo:         [self drawCombo:item]; break;
        case app::DeviceWidget::meter:         [self drawMeter:item]; break;
        case app::DeviceWidget::faderWall:     [self drawFaderWall:item]; break;
        case app::DeviceWidget::drawableCurve: [self drawCurve:item]; break;
        default:                               [self drawPlaceholder:item]; break;
        }
    }
}

- (void)drawSection:(const app::DeviceUiPlacement&)item
{
    const NSRect header = [self rectFor:item.caption];

    theme::drawSeparator(NSMakeRect(NSMinX(header), NSMaxY(header) - 2.0, header.size.width,
                                    1.0));

    NSString* title = [NSString stringWithFormat:@"%@ %@",
                                                 [self isSectionCollapsed:*item.widget] ? @"▸"
                                                                                       : @"▾",
                                                 text(item.widget->label).uppercaseString];

    theme::drawTextCentred(title, NSInsetRect(header, 0.0, 3.0), theme::ink(Ink::textSecondary),
                           theme::labelFont(10.5, NSFontWeightSemibold));
}

- (void)drawTabHeader:(const app::DeviceUiPlacement&)item
{
    theme::drawTextCentred(text(item.widget->label).uppercaseString,
                           NSInsetRect([self rectFor:item.caption], 0.0, 3.0),
                           theme::ink(Ink::textSecondary),
                           theme::labelFont(10.5, NSFontWeightSemibold));
}

- (void)drawLabel:(const app::DeviceUiPlacement&)item
{
    theme::drawTextCentred(text(item.widget->label), [self rectFor:item.caption],
                           theme::ink(Ink::textSecondary), theme::labelFont(11.0));
}

- (void)drawKnob:(const app::DeviceUiPlacement&)item
{
    const app::DeviceUiWidget&     widget    = *item.widget;
    const app::DeviceUiParameter* parameter = [self firstParameterOf:widget];
    if (parameter == nullptr)
        return;

    theme::drawTextCentred(text(widget.label).uppercaseString, [self rectFor:item.caption],
                           theme::ink(Ink::textSecondary),
                           theme::labelFont(10.5, NSFontWeightSemibold), theme::Align::centre);

    const app::DeviceUiRange range = app::effectiveRange(widget, *parameter);
    const bool atRest = widget.bipolar ? parameter->value == 0.0
                                       : parameter->value == parameter->defaultValue;

    NSColor* accent = inkForToken(widget.tint, Ink::accent);
    theme::drawKnob([self rectFor:item.control], app::toNormalised(parameter->value, range),
                    atRest ? theme::ink(Ink::accentDim) : accent, widget.bipolar);

    theme::drawTextCentred(text(app::formatDeviceValue(parameter->value, widget, *parameter)),
                           [self rectFor:item.readout], theme::ink(Ink::lcdText),
                           theme::numericFont(12.0), theme::Align::centre);
}

- (void)drawSlider:(const app::DeviceUiPlacement&)item
{
    const app::DeviceUiWidget&     widget    = *item.widget;
    const app::DeviceUiParameter* parameter = [self firstParameterOf:widget];
    if (parameter == nullptr)
        return;

    theme::drawTextCentred(text(widget.label), [self rectFor:item.caption],
                           theme::ink(Ink::textDim), theme::labelFont(10.5));

    theme::drawSlider([self rectFor:item.control],
                      app::toNormalised(parameter->value, app::effectiveRange(widget, *parameter)),
                      inkForToken(widget.tint, Ink::accentDim));

    theme::drawTextCentred(text(app::formatDeviceValue(parameter->value, widget, *parameter)),
                           [self rectFor:item.readout], theme::ink(Ink::lcdText),
                           theme::numericFont(10.0), theme::Align::right);
}

- (void)drawToggle:(const app::DeviceUiPlacement&)item
{
    const app::DeviceUiWidget&     widget    = *item.widget;
    const app::DeviceUiParameter* parameter = [self firstParameterOf:widget];
    if (parameter == nullptr)
        return;

    theme::drawToggle([self rectFor:item.control], nil, parameter->value >= 0.5,
                      inkForToken(widget.tint, Ink::accent));
    theme::drawTextCentred(text(widget.label), [self rectFor:item.caption],
                           theme::ink(Ink::textSecondary), theme::labelFont(10.5));
}

- (void)drawCombo:(const app::DeviceUiPlacement&)item
{
    const app::DeviceUiWidget&     widget    = *item.widget;
    const app::DeviceUiParameter* parameter = [self firstParameterOf:widget];
    if (parameter == nullptr)
        return;

    theme::drawTextCentred(text(widget.label), [self rectFor:item.caption],
                           theme::ink(Ink::textDim), theme::labelFont(10.5));

    const NSRect box = [self rectFor:item.control];
    theme::drawPanel(box, theme::metrics::radiusControl, false, false);

    theme::drawTextCentred(text(app::formatDeviceValue(parameter->value, widget, *parameter)),
                           NSInsetRect(box, 8.0, 0.0), theme::ink(Ink::textPrimary),
                           theme::labelFont(11.0));
    theme::drawTextCentred(@"▾", NSMakeRect(NSMaxX(box) - 18.0, NSMinY(box), 14.0,
                                            box.size.height),
                           theme::ink(Ink::textSecondary), theme::labelFont(10.0),
                           theme::Align::centre);
}

- (void)drawMeter:(const app::DeviceUiPlacement&)item
{
    const app::DeviceUiWidget&     widget    = *item.widget;
    const app::DeviceUiParameter* parameter = [self firstParameterOf:widget];

    theme::drawTextCentred(text(widget.label), [self rectFor:item.caption],
                           theme::ink(Ink::textDim), theme::labelFont(10.5));

    const NSRect housing = [self rectFor:item.control];
    theme::drawWell(housing, theme::metrics::radiusPad);

    const double level = parameter != nullptr
                           ? app::toNormalised(parameter->value,
                                               app::effectiveRange(widget, *parameter))
                           : 0.0;
    theme::drawMeter(NSInsetRect(housing, 2.0, 2.0), level, level, false);

    if (parameter != nullptr)
        theme::drawTextCentred(text(app::formatDeviceValue(parameter->value, widget, *parameter)),
                               [self rectFor:item.readout], theme::ink(Ink::lcdText),
                               theme::numericFont(10.0), theme::Align::right);
}

- (NSRect)faderRect:(std::size_t)index of:(const app::DeviceUiPlacement&)item
{
    const NSRect well  = [self rectFor:item.control];
    const std::size_t count = std::max<std::size_t>(item.widget->parameters.size(), 1);
    const CGFloat pitch = (well.size.width - 12.0) / static_cast<CGFloat>(count);
    const CGFloat width = std::min<CGFloat>(18.0, pitch - 2.0);

    return NSMakeRect(NSMinX(well) + 6.0 + pitch * static_cast<CGFloat>(index)
                          + (pitch - width) / 2.0,
                      NSMinY(well) + 8.0, width,
                      well.size.height - app::layout::captionHeight - 16.0);
}

- (void)drawFaderWall:(const app::DeviceUiPlacement&)item
{
    const app::DeviceUiWidget& widget = *item.widget;

    theme::drawWell([self rectFor:item.control], theme::metrics::radiusPanel);
    theme::drawTextCentred(text(widget.label).uppercaseString, [self rectFor:item.caption],
                           theme::ink(Ink::textDim), theme::labelFont(10.0, NSFontWeightSemibold));

    NSColor* accent = inkForToken(widget.tint, Ink::accent);

    for (std::size_t index = 0; index < widget.parameters.size(); ++index) {
        const app::DeviceUiParameter* parameter = [self parameterFor:widget.parameters[index]];
        if (parameter == nullptr)
            continue;

        theme::drawFader([self faderRect:index of:item],
                         app::toNormalised(parameter->value,
                                           app::effectiveRange(widget, *parameter)),
                         accent, false);
    }
}

- (void)drawPlaceholder:(const app::DeviceUiPlacement&)item
{
    theme::drawWell([self rectFor:item.control], theme::metrics::radiusPanel);

    NSString* caption = text(item.widget->label);
    if (caption.length == 0)
        caption = text(item.widget->plot);

    theme::drawTextCentred(caption, [self rectFor:item.caption], theme::ink(Ink::textDim),
                           theme::labelFont(10.5));
}

/// The response curve, plotted from the engine's own band design when the
/// widget carries the EQ's seven parameters; any other plot is a well with
/// its name until the renderer learns it.
///
/// The grid, the curve and the handles are all placed on the SAME resolved
/// axes (app::plotAxes), so a handle cannot float off the line it belongs to
/// when a spec states axes of its own.
- (void)drawCurve:(const app::DeviceUiPlacement&)item
{
    const app::DeviceUiWidget& widget = *item.widget;
    const NSRect well = [self rectFor:item.control];
    theme::drawWell(well, theme::metrics::radiusPanel);

    double params[dsp::EqEffect::paramCount] = {};
    bool   seen[dsp::EqEffect::paramCount]   = {};

    if (widget.plot == "eq-response")
        for (std::uint32_t id : widget.parameters)
            if (id < dsp::EqEffect::paramCount)
                if (const app::DeviceUiParameter* parameter = [self parameterFor:id]) {
                    params[id] = parameter->value;
                    seen[id]   = true;
                }

    const bool plottable = widget.plot == "eq-response"
                        && std::all_of(std::begin(seen), std::end(seen), [](bool s) { return s; });

    if (!plottable) {
        [self drawPlaceholder:item];
        return;
    }

    const app::DeviceUiRect plot = [self plotRectOf:item];
    const app::DeviceUiAxes axes = [self axesOf:widget];

    static const double decades[] = {50, 100, 200, 500, 1000, 2000, 5000, 10000};

    for (double hz : decades) {
        if (hz < axes.x.min || hz > axes.x.max)
            continue;

        const bool    labelled = hz == 100.0 || hz == 1000.0 || hz == 10000.0;
        const CGFloat x        = static_cast<CGFloat>(app::axisPositionX(hz, axes.x, plot));

        theme::fillRect(NSMakeRect(x, [self viewYFor:plot.bottom()], 1.0, plot.height),
                        theme::ink(labelled ? Ink::gridLineStrong : Ink::gridLine));

        if (labelled)
            theme::drawTextCentred(hz >= 1000.0 ? [NSString stringWithFormat:@"%.0fk", hz / 1000.0]
                                                : [NSString stringWithFormat:@"%.0f", hz],
                                   NSMakeRect(x - 24.0, [self viewYFor:plot.bottom()], 48.0, 12.0),
                                   theme::ink(Ink::textDim), theme::numericFont(9.0),
                                   theme::Align::centre);
    }

    // Half-way to each end, and the zero line the shelves hinge on.
    const double half = axes.y.max / 2.0;

    for (double db : {-half, 0.0, half}) {
        const CGFloat y = [self viewYFor:app::axisPositionY(db, axes.y, plot)];
        theme::fillRect(NSMakeRect(plot.x, y, plot.width, 1.0),
                        theme::ink(db == 0.0 ? Ink::gridLineStrong : Ink::gridLine));
    }

    NSBezierPath* curve = [NSBezierPath bezierPath];
    NSBezierPath* under = [NSBezierPath bezierPath];

    const CGFloat left  = static_cast<CGFloat>(plot.x);
    const CGFloat right = static_cast<CGFloat>(plot.right());
    const CGFloat midY  = [self viewYFor:app::axisPositionY(0.0, axes.y, plot)];

    for (CGFloat x = left; x <= right; x += 1.0) {
        const double hz = app::axisValueX(x, axes.x, plot);
        const double db = std::clamp(dsp::eqMagnitudeDb(params, _sampleRate, hz), axes.y.min,
                                     axes.y.max);

        const NSPoint point = NSMakePoint(x, [self viewYFor:app::axisPositionY(db, axes.y, plot)]);

        if (x == left) {
            [curve moveToPoint:point];
            [under moveToPoint:NSMakePoint(x, midY)];
            [under lineToPoint:point];
        } else {
            [curve lineToPoint:point];
            [under lineToPoint:point];
        }
    }

    [under lineToPoint:NSMakePoint(right, midY)];
    [under closePath];

    NSColor* accent = inkForToken(widget.tint, Ink::accent);
    [theme::withAlpha(accent, 0.18) setFill];
    [under fill];

    curve.lineWidth = 2.0;
    [accent setStroke];
    [curve stroke];

    [self drawHandlesOf:item axes:axes plot:plot accent:accent];
}

/// A band's handle: a filled disc with a ring, its name under it. The held
/// one is drawn larger, so a drag is legible without a readout.
- (void)drawHandlesOf:(const app::DeviceUiPlacement&)item
                 axes:(const app::DeviceUiAxes&)axes
                 plot:(const app::DeviceUiRect&)plot
               accent:(NSColor*)accent
{
    const app::DeviceUiWidget& widget = *item.widget;
    const std::function<double(std::uint32_t)> valueOf = [self valueLookup];

    for (std::size_t index = 0; index < widget.points.size(); ++index) {
        const app::DeviceUiRect cell =
            app::handleRect(widget.points[index], axes, plot, valueOf);

        const bool held = _drag == Drag::handle && _dragItem < _layout.items.size()
                       && _layout.items[_dragItem].widget == item.widget && _dragHandle == index;

        const CGFloat grow = held ? 2.0 : 0.0;
        const NSRect  disc = NSInsetRect(NSMakeRect(cell.x, [self viewYFor:cell.bottom()],
                                                    cell.width, cell.height),
                                         -grow, -grow);

        NSBezierPath* path = [NSBezierPath bezierPathWithOvalInRect:disc];

        [accent setFill];
        [path fill];

        path.lineWidth = 1.5;
        [theme::ink(Ink::panel) setStroke];
        [path stroke];

        if (!widget.points[index].label.empty())
            theme::drawTextCentred(text(widget.points[index].label),
                                   NSMakeRect(NSMidX(disc) - 24.0, NSMinY(disc) - 12.0, 48.0,
                                              11.0),
                                   theme::ink(Ink::textDim), theme::labelFont(9.0),
                                   theme::Align::centre);
    }
}

// ── Editing ──────────────────────────────────────────────────────────────────

- (void)write:(std::uint32_t)parameterId
        value:(double)value
       widget:(const app::DeviceUiWidget&)widget
{
    app::DeviceUiParameter* parameter = [self parameterFor:parameterId];
    if (parameter == nullptr)
        return;

    const double constrained = app::constrainValue(value, widget, *parameter);
    if (constrained == parameter->value)
        return;

    parameter->value = constrained;

    if (_onWrite != nil)
        _onWrite(parameterId, constrained);

    self.needsDisplay = YES;
}

/// The layout item under `point` (in layout space), hit-tested on the
/// control rect with `inset` of slack, last placed first — the way a child
/// sits over its container.
- (std::size_t)itemAt:(NSPoint)point inset:(double)inset
{
    for (std::size_t index = _layout.items.size(); index-- > 0;) {
        const app::DeviceUiPlacement& item = _layout.items[index];
        if (!item.visible || item.control.width <= 0.0)
            continue;

        const app::DeviceWidget kind = item.widget->kind;
        if (kind == app::DeviceWidget::row || kind == app::DeviceWidget::grid
            || kind == app::DeviceWidget::tab)
            continue;

        app::DeviceUiRect slack = item.control;
        slack.x -= inset;
        slack.y -= inset;
        slack.width += inset * 2.0;
        slack.height += inset * 2.0;

        if (slack.contains(point.x, point.y))
            return index;
    }

    return _layout.items.size();
}

- (NSPoint)layoutPointFor:(NSEvent*)event
{
    const NSPoint view = [self convertPoint:event.locationInWindow fromView:nil];
    return NSMakePoint(view.x, self.bounds.size.height - view.y);
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint point = [self layoutPointFor:event];
    _drag = Drag::none;

    const std::size_t index = [self itemAt:point inset:6.0];
    if (index >= _layout.items.size())
        return;

    const app::DeviceUiPlacement& item   = _layout.items[index];
    const app::DeviceUiWidget&    widget = *item.widget;

    switch (widget.kind) {
    case app::DeviceWidget::section:
        if ([self isSectionCollapsed:widget]) {
            _folded.erase(&widget);
            _expanded.insert(&widget);
        } else {
            _expanded.erase(&widget);
            _folded.insert(&widget);
        }
        [self relayoutAndResize];
        return;

    case app::DeviceWidget::knob: {
        const app::DeviceUiParameter* parameter = [self firstParameterOf:widget];
        if (parameter == nullptr)
            return;

        if (event.clickCount == 2) {
            [self write:parameter->id value:app::resetValue(widget, *parameter) widget:widget];
            return;
        }

        _drag           = Drag::knob;
        _dragItem       = index;
        _dragParameter  = parameter->id;
        _dragStartValue = parameter->value;
        _dragStartPoint = point;
        return;
    }

    case app::DeviceWidget::slider:
        if (const app::DeviceUiParameter* parameter = [self firstParameterOf:widget]) {
            if (event.clickCount == 2) {
                [self write:parameter->id
                      value:app::resetValue(widget, *parameter)
                     widget:widget];
                return;
            }

            _drag          = Drag::slider;
            _dragItem      = index;
            _dragParameter = parameter->id;
            [self applySliderAt:point item:item];
        }
        return;

    case app::DeviceWidget::toggle:
        if (const app::DeviceUiParameter* parameter = [self firstParameterOf:widget])
            [self write:parameter->id value:parameter->value >= 0.5 ? 0.0 : 1.0 widget:widget];
        return;

    case app::DeviceWidget::combo:
        [self popUpComboFor:item at:event];
        return;

    case app::DeviceWidget::drawableCurve: {
        const app::DeviceUiRect plot = [self plotRectOf:item];
        const app::DeviceUiAxes axes = [self axesOf:widget];

        const std::optional<std::size_t> handle =
            app::handleAt(widget, axes, plot, point.x, point.y, [self valueLookup]);

        // A curve with no handle under the pointer is a picture, not a
        // control: the click does nothing rather than jumping a band.
        if (!handle.has_value())
            return;

        if (event.clickCount == 2) {
            [self resetHandle:widget.points[*handle] widget:widget];
            return;
        }

        _drag       = Drag::handle;
        _dragItem   = index;
        _dragHandle = *handle;

        self.needsDisplay = YES;
        return;
    }

    case app::DeviceWidget::faderWall: {
        const std::size_t fader = [self faderIndexAt:point item:item];
        if (fader >= widget.parameters.size())
            return;

        _drag          = Drag::fader;
        _dragItem      = index;
        _dragFader     = fader;
        _dragParameter = widget.parameters[fader];
        [self applyFaderAt:point item:item fader:fader];
        return;
    }

    default:
        return;
    }
}

- (void)mouseDragged:(NSEvent*)event
{
    if (_drag == Drag::none || _dragItem >= _layout.items.size())
        return;

    const NSPoint                 point = [self layoutPointFor:event];
    const app::DeviceUiPlacement& item  = _layout.items[_dragItem];

    switch (_drag) {
    case Drag::slider:
        [self applySliderAt:point item:item];
        return;

    case Drag::fader:
        [self applyFaderAt:point item:item fader:_dragFader];
        return;

    case Drag::knob: {
        const app::DeviceUiParameter* parameter = [self parameterFor:_dragParameter];
        if (parameter == nullptr)
            return;

        const app::DeviceUiRange range = app::effectiveRange(*item.widget, *parameter);
        const double fine = (event.modifierFlags & NSEventModifierFlagOption) != 0 ? fineFactor
                                                                                   : 1.0;

        // Layout y grows down, so dragging UP is a negative delta and must
        // raise the value.
        const double travel = (_dragStartPoint.y - point.y) / knobTravel * fine;
        const double start  = app::toNormalised(_dragStartValue, range);

        [self write:_dragParameter
              value:app::fromNormalised(start + travel, range)
             widget:*item.widget];
        return;
    }

    case Drag::handle: {
        const app::DeviceUiWidget& widget = *item.widget;
        if (_dragHandle >= widget.points.size())
            return;

        [self apply:app::handleDrag(widget.points[_dragHandle], widget, [self axesOf:widget],
                                    [self plotRectOf:item], point.x, point.y,
                                    [self parameterLookup])
             widget:widget];
        return;
    }

    case Drag::none:
        return;
    }
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;
    _drag             = Drag::none;
    self.needsDisplay = YES;   // the held handle shrinks back
}

/// The wheel over a handle moves its third axis — a band's Q, a point's
/// tension — along its own travel, so one tick is the same gesture whatever
/// the parameter is. Over anything else the scroll belongs to the panel's
/// scroll view, so it is passed on.
- (void)scrollWheel:(NSEvent*)event
{
    const NSPoint     point = [self layoutPointFor:event];
    const std::size_t index = [self itemAt:point inset:6.0];

    if (index >= _layout.items.size()
        || _layout.items[index].widget->kind != app::DeviceWidget::drawableCurve) {
        [super scrollWheel:event];
        return;
    }

    const app::DeviceUiPlacement& item   = _layout.items[index];
    const app::DeviceUiWidget&    widget = *item.widget;

    const std::optional<std::size_t> handle =
        app::handleAt(widget, [self axesOf:widget], [self plotRectOf:item], point.x, point.y,
                      [self valueLookup]);

    if (!handle.has_value()) {
        [super scrollWheel:event];
        return;
    }

    const std::vector<app::DeviceUiWrite> writes =
        app::handleScroll(widget.points[*handle], widget, event.scrollingDeltaY,
                          [self parameterLookup]);

    if (writes.empty()) {
        [super scrollWheel:event];
        return;
    }

    [self apply:writes widget:widget];
}

/// Double-clicking a handle puts the axes it moves back to the table's
/// defaults. Its `z` is left alone: the wheel is a separate gesture.
- (void)resetHandle:(const app::DeviceUiPoint&)handle
             widget:(const app::DeviceUiWidget&)widget
{
    for (const std::optional<std::uint32_t>& id : {handle.x, handle.y})
        if (id.has_value())
            if (const app::DeviceUiParameter* parameter = [self parameterFor:*id])
                [self write:*id value:app::resetValue(widget, *parameter) widget:widget];
}

- (void)applySliderAt:(NSPoint)point item:(const app::DeviceUiPlacement&)item
{
    const app::DeviceUiParameter* parameter = [self parameterFor:_dragParameter];
    if (parameter == nullptr)
        return;

    const double normalised = item.control.width > 0.0
                                ? (point.x - item.control.x) / item.control.width
                                : 0.0;

    [self write:_dragParameter
          value:app::fromNormalised(normalised, app::effectiveRange(*item.widget, *parameter))
         widget:*item.widget];
}

- (std::size_t)faderIndexAt:(NSPoint)point item:(const app::DeviceUiPlacement&)item
{
    const std::size_t count = item.widget->parameters.size();
    if (count == 0 || item.control.width <= 12.0)
        return count;

    const double pitch = (item.control.width - 12.0) / static_cast<double>(count);
    const double local = point.x - item.control.x - 6.0;
    if (local < 0.0)
        return 0;

    return std::min(count - 1, static_cast<std::size_t>(local / pitch));
}

- (void)applyFaderAt:(NSPoint)point item:(const app::DeviceUiPlacement&)item fader:(std::size_t)fader
{
    const app::DeviceUiParameter* parameter = [self parameterFor:_dragParameter];
    if (parameter == nullptr)
        return;

    // The fader's travel in layout space: from the well's top caption band
    // down to its foot, matching faderRect:of:.
    const double top    = item.control.y + app::layout::captionHeight + 8.0;
    const double height = item.control.height - app::layout::captionHeight - 16.0;
    const double normalised = height > 0.0 ? 1.0 - (point.y - top) / height : 0.0;

    (void)fader;
    [self write:_dragParameter
          value:app::fromNormalised(normalised, app::effectiveRange(*item.widget, *parameter))
         widget:*item.widget];
}

- (void)popUpComboFor:(const app::DeviceUiPlacement&)item at:(NSEvent*)event
{
    const app::DeviceUiWidget&     widget    = *item.widget;
    const app::DeviceUiParameter* parameter = [self firstParameterOf:widget];
    if (parameter == nullptr)
        return;

    _menuParameter = parameter->id;

    NSMenu* menu = [[NSMenu alloc] init];

    // Named choices when the spec has them; otherwise the stepped range.
    const long first = static_cast<long>(std::lround(parameter->minValue));
    const long last  = static_cast<long>(std::lround(parameter->maxValue));

    for (long value = first; value <= last; ++value) {
        const auto choice = static_cast<std::size_t>(value - first);
        NSString*  name   = choice < widget.choices.size()
                              ? text(widget.choices[choice])
                              : [NSString stringWithFormat:@"%ld", value];

        NSMenuItem* entry = [menu addItemWithTitle:name
                                            action:@selector(comboChosen:)
                                     keyEquivalent:@""];
        entry.target            = self;
        entry.representedObject = @(static_cast<double>(value));
        entry.state = std::lround(parameter->value) == value ? NSControlStateValueOn
                                                             : NSControlStateValueOff;
    }

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

- (void)comboChosen:(NSMenuItem*)entry
{
    for (const app::DeviceUiPlacement& item : _layout.items)
        if (item.widget->kind == app::DeviceWidget::combo && !item.widget->parameters.empty()
            && item.widget->parameters.front() == _menuParameter) {
            [self write:_menuParameter
                  value:[entry.representedObject doubleValue]
                 widget:*item.widget];
            return;
        }
}

/// Grows and shrinks the window around a disclosure, anchored at its title
/// bar; a panel inside a scroll view resizes its document instead.
- (void)relayoutAndResize
{
    [self relayout];

    const CGFloat wanted = static_cast<CGFloat>(_layout.height);
    const CGFloat delta  = wanted - self.bounds.size.height;

    if (self.enclosingScrollView != nil) {
        [self setFrameSize:NSMakeSize(self.bounds.size.width, wanted)];
        self.needsDisplay = YES;
        return;
    }

    if (NSWindow* window = self.window; window != nil && delta != 0.0) {
        NSRect frame = window.frame;
        frame.origin.y    -= delta;
        frame.size.height += delta;
        [window setFrame:frame display:YES];
    } else {
        [self setFrameSize:NSMakeSize(self.bounds.size.width, wanted)];
    }

    self.needsDisplay = YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

@end

// ── The panel ────────────────────────────────────────────────────────────────

@implementation INCDAWDevicePanel

+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    spec:(const app::DeviceUiSpec*)spec
                                    rows:(NSArray<NSDictionary*>*)rows
                              sampleRate:(double)sampleRate
                                 onWrite:(void (^)(std::uint32_t, double))onWrite
{
    if (spec == nullptr)
        return nil;

    NSView* content = nil;

    if (!spec->customView.empty()) {
        Class custom = NSClassFromString(text(spec->customView));
        if (custom == Nil || ![custom conformsToProtocol:@protocol(INCDAWDeviceCustomView)])
            return nil;   // named but not linked: the generic panel is the fallback

        content = [custom makeViewWithRows:rows sampleRate:sampleRate onWrite:onWrite];
        if (content == nil)
            return nil;
    } else {
        INCDAWDeviceView* view = [[INCDAWDeviceView alloc] initWithFrame:NSMakeRect(0, 0, 10, 10)];
        view->_spec       = spec;
        view->_sampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
        view->_onWrite    = onWrite;

        for (NSDictionary* row in rows) {
            app::DeviceUiParameter parameter;
            parameter.id           = [row[@"id"] unsignedIntValue];
            parameter.minValue     = [row[@"min"] doubleValue];
            parameter.maxValue     = [row[@"max"] doubleValue];
            parameter.value        = [row[@"value"] doubleValue];
            parameter.stepped      = [row[@"stepped"] boolValue];
            parameter.defaultValue = row[@"default"] != nil ? [row[@"default"] doubleValue]
                                                             : parameter.value;
            view->_params[parameter.id] = parameter;
        }

        // Every parameter the spec names must be one the device has; a spec
        // that outruns its device is a bug, and the generic panel still works.
        const std::function<bool(const app::DeviceUiWidget&)> resolvable =
            [&](const app::DeviceUiWidget& widget) -> bool {
            for (std::uint32_t id : widget.parameters)
                if (view->_params.count(id) == 0)
                    return false;

            // A handle's axes are parameters too: a point naming an id the
            // device lacks would ship as a control that writes nowhere, so
            // the whole spec falls back to the generic panel instead.
            for (const app::DeviceUiPoint& point : widget.points)
                for (const std::optional<std::uint32_t>& id : {point.x, point.y, point.z})
                    if (id.has_value() && view->_params.count(*id) == 0)
                        return false;

            for (const app::DeviceUiWidget& child : widget.children)
                if (!resolvable(child))
                    return false;
            return true;
        };

        for (const app::DeviceUiWidget& widget : spec->root)
            if (!resolvable(widget)) {
                NSLog(@"INCDAW: device spec %s names a parameter the device does not carry; "
                      @"using the generic panel",
                      spec->uid.c_str());
                return nil;
            }

        [view relayout];
        [view setFrameSize:NSMakeSize(view->_layout.width, view->_layout.height)];
        content = view;
    }

    const CGFloat contentHeight = content.frame.size.height;
    const CGFloat contentWidth  = content.frame.size.width;
    const CGFloat visibleHeight = std::min(contentHeight, visibleLimit);

    NSView* windowContent = content;

    if (contentHeight > visibleLimit) {
        NSScrollView* scroll =
            [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, contentWidth, visibleHeight)];
        scroll.hasVerticalScroller = YES;
        scroll.documentView        = content;
        scroll.drawsBackground     = YES;
        scroll.backgroundColor     = theme::ink(Ink::panel);
        windowContent              = scroll;
    }

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, contentWidth, visibleHeight)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    window.releasedWhenClosed = NO;
    window.title              = title;
    window.contentView        = windowContent;
    window.appearance         = [NSAppearance appearanceNamed:theme::paletteIsLight()
                                                                  ? NSAppearanceNameAqua
                                                                  : NSAppearanceNameDarkAqua];
    window.backgroundColor    = theme::ink(Ink::windowBackground);

    objc_setAssociatedObject(window, panelOwnerKey, content, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return window;
}

+ (BOOL)isDevicePanel:(NSWindow*)window
{
    return objc_getAssociatedObject(window, panelOwnerKey) != nil;
}

+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values
{
    NSView* content = objc_getAssociatedObject(window, panelOwnerKey);
    if (content == nil)
        return;

    // A drag in progress owns the controls; the next tick catches up.
    if ((NSEvent.pressedMouseButtons & 1) != 0)
        return;

    if ([content conformsToProtocol:@protocol(INCDAWDeviceCustomView)]) {
        [static_cast<id<INCDAWDeviceCustomView>>(content) refreshValues:values];
        return;
    }

    if (![content isKindOfClass:[INCDAWDeviceView class]])
        return;

    INCDAWDeviceView* view    = static_cast<INCDAWDeviceView*>(content);
    BOOL              changed = NO;

    for (auto& [id, parameter] : view->_params) {
        NSNumber* incoming = values[@(id)];
        if (incoming == nil || incoming.doubleValue == parameter.value)
            continue;

        parameter.value = incoming.doubleValue;
        changed         = YES;
    }

    if (changed)
        view.needsDisplay = YES;
}

+ (void)refreshAppearance:(NSWindow*)window
{
    NSView* content = objc_getAssociatedObject(window, panelOwnerKey);
    if (content == nil)
        return;

    if ([content.enclosingScrollView isKindOfClass:[NSScrollView class]])
        content.enclosingScrollView.backgroundColor = theme::ink(Ink::panel);

    theme::refreshViewTree(content);
}

@end
