#import "ui/macos/TonePanel.h"

#include "engine/dsp/effects/ToneEffects.h"
#include "ui/macos/Theme.h"

#include <algorithm>
#include <cmath>

namespace theme = incdaw::ui::theme;
namespace dsp   = incdaw::engine::dsp;

using incdaw::ui::theme::Ink;

namespace {

// ── Geometry ─────────────────────────────────────────────────────────────────
//
// The window is fixed: three knobs over a curve is a shape, not a list, and a
// resizable version would only stretch whitespace.

constexpr CGFloat panelWidth      = 468.0;
constexpr CGFloat collapsedHeight = 344.0;   ///< curve, knobs, the disclosure
constexpr CGFloat expandedHeight  = 446.0;   ///< and the four advanced rows
constexpr CGFloat margin          = 14.0;

constexpr CGFloat curveTop    = 14.0;
constexpr CGFloat curveHeight = 156.0;

constexpr CGFloat bandLabelTop = 182.0;
constexpr CGFloat knobTop      = 200.0;
constexpr CGFloat knobSize     = 62.0;
constexpr CGFloat gainTop      = 268.0;
constexpr CGFloat freqTop      = 288.0;

constexpr CGFloat sectionTop   = 312.0;
constexpr CGFloat advancedTop  = 336.0;
constexpr CGFloat advancedRow  = 24.0;

constexpr CGFloat displayDb    = 24.0;   ///< the curve's vertical half-range
constexpr CGFloat minPlotHz    = 20.0;
constexpr CGFloat maxPlotHz    = 20000.0;

/// The three knobs, and the four rows behind the disclosure. Ids are the
/// EqEffect::Param values, which are also the indices of the value array.
struct BandSpec {
    const char*   caption;
    std::uint32_t gainId;
    std::uint32_t freqId;
};

constexpr BandSpec bands[3] = {
    {"BASS",   dsp::EqEffect::lowGainDb,  dsp::EqEffect::lowFreq},
    {"MID",    dsp::EqEffect::midGainDb,  dsp::EqEffect::midFreq},
    {"TREBLE", dsp::EqEffect::highGainDb, dsp::EqEffect::highFreq},
};

constexpr std::uint32_t advancedIds[4] = {
    dsp::EqEffect::lowFreq,
    dsp::EqEffect::midFreq,
    dsp::EqEffect::midQ,
    dsp::EqEffect::highFreq,
};

/// Gains snap to flat inside this window, so "back to neutral" is a gesture
/// rather than a hunt for the exact pixel.
constexpr double detentDb = 0.6;

[[nodiscard]] CGFloat columnX(std::size_t index)
{
    const CGFloat width = (panelWidth - margin * 2.0) / 3.0;
    return margin + width * static_cast<CGFloat>(index);
}

[[nodiscard]] CGFloat columnWidth()
{
    return (panelWidth - margin * 2.0) / 3.0;
}

[[nodiscard]] NSString* gainText(double db)
{
    return db == 0.0 ? @"0.0 dB" : [NSString stringWithFormat:@"%+.1f dB", db];
}

[[nodiscard]] NSString* frequencyText(double hz)
{
    return hz >= 1000.0 ? [NSString stringWithFormat:@"%.2f kHz", hz / 1000.0]
                        : [NSString stringWithFormat:@"%.0f Hz", hz];
}

/// A frequency parameter reads as a fader only on a log axis; Q does not.
[[nodiscard]] bool isLogarithmic(std::uint32_t parameterId)
{
    return parameterId != dsp::EqEffect::midQ;
}

[[nodiscard]] double toNormalised(double value, double minValue, double maxValue,
                                  bool logarithmic)
{
    if (maxValue <= minValue)
        return 0.0;

    if (logarithmic && minValue > 0.0)
        return std::log(std::max(value, minValue) / minValue) / std::log(maxValue / minValue);

    return (value - minValue) / (maxValue - minValue);
}

[[nodiscard]] double fromNormalised(double normalised, double minValue, double maxValue,
                                    bool logarithmic)
{
    const double clamped = std::clamp(normalised, 0.0, 1.0);

    if (logarithmic && minValue > 0.0)
        return minValue * std::pow(maxValue / minValue, clamped);

    return minValue + (maxValue - minValue) * clamped;
}

enum class Drag { none, knob, advanced };

} // namespace

// ── The view ─────────────────────────────────────────────────────────────────

@interface INCDAWToneView : NSView
@end

@implementation INCDAWToneView {
@public
    double _values[dsp::EqEffect::paramCount];
    double _minima[dsp::EqEffect::paramCount];
    double _maxima[dsp::EqEffect::paramCount];
    double _sampleRate;
    BOOL   _advancedShown;

    Drag        _drag;
    std::size_t _dragIndex;
    double      _dragStartValue;
    NSPoint     _dragStartPoint;

    void (^_onWrite)(std::uint32_t, double);
}

- (instancetype)initWithFrame:(NSRect)frame
{
    if ((self = [super initWithFrame:frame]) != nil) {
        _sampleRate    = 48000.0;
        _advancedShown = NO;
        _drag          = Drag::none;
    }

    return self;
}

// ── Rects (drawing and hit testing read the same ones) ───────────────────────

/// Everything is placed from the view's TOP edge, so collapsing the advanced
/// section can shorten the window without moving a single control.
- (NSRect)rectFromTop:(CGFloat)top height:(CGFloat)height x:(CGFloat)x width:(CGFloat)width
{
    return NSMakeRect(x, NSMaxY(self.bounds) - top - height, width, height);
}

- (NSRect)curveRect
{
    return [self rectFromTop:curveTop height:curveHeight x:margin width:panelWidth - margin * 2.0];
}

- (NSRect)knobRectAt:(std::size_t)index
{
    return [self rectFromTop:knobTop
                      height:knobSize
                           x:columnX(index) + (columnWidth() - knobSize) / 2.0
                       width:knobSize];
}

- (NSRect)advancedToggleRect
{
    return [self rectFromTop:sectionTop height:16.0 x:margin width:120.0];
}

- (NSRect)resetRect
{
    return [self rectFromTop:sectionTop - 2.0
                      height:20.0
                           x:panelWidth - margin - 66.0
                       width:66.0];
}

- (NSRect)advancedSliderRectAt:(std::size_t)row
{
    const CGFloat top = advancedTop + advancedRow * static_cast<CGFloat>(row);
    const CGFloat x   = margin + 92.0;

    return [self rectFromTop:top + 5.0
                      height:12.0
                           x:x
                       width:panelWidth - margin - 74.0 - x];
}

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    theme::fillRect(self.bounds, theme::ink(Ink::panel));

    [self drawCurve];
    [self drawBands];
    [self drawSection];

    if (_advancedShown)
        [self drawAdvanced];
}

- (void)drawCurve
{
    const NSRect well = [self curveRect];
    theme::drawWell(well, theme::metrics::radiusPanel, false);

    const NSRect plot = NSInsetRect(well, 6.0, 6.0);
    const CGFloat midY = NSMidY(plot);
    const CGFloat scale = plot.size.height / 2.0 / displayDb;

    const auto xForHz = [&](double hz) {
        const double t = std::log(hz / minPlotHz) / std::log(maxPlotHz / minPlotHz);
        return NSMinX(plot) + plot.size.width * static_cast<CGFloat>(t);
    };

    // ── Grid ─────────────────────────────────────────────────────────────────
    static const double decades[] = {50, 100, 200, 500, 1000, 2000, 5000, 10000};

    for (double hz : decades) {
        const bool labelled = hz == 100.0 || hz == 1000.0 || hz == 10000.0;
        const CGFloat x = xForHz(hz);

        theme::fillRect(NSMakeRect(x, NSMinY(plot), 1.0, plot.size.height),
                        theme::ink(labelled ? Ink::gridLineStrong : Ink::gridLine));

        if (labelled)
            theme::drawTextCentred(hz >= 1000.0
                                     ? [NSString stringWithFormat:@"%.0fk", hz / 1000.0]
                                     : [NSString stringWithFormat:@"%.0f", hz],
                                   NSMakeRect(x - 24.0, NSMinY(plot), 48.0, 12.0),
                                   theme::ink(Ink::textDim), theme::numericFont(9.0),
                                   theme::Align::centre);
    }

    for (double db : {-12.0, 0.0, 12.0}) {
        const CGFloat y = midY + static_cast<CGFloat>(db) * scale;
        theme::fillRect(NSMakeRect(NSMinX(plot), y, plot.size.width, 1.0),
                        theme::ink(db == 0.0 ? Ink::gridLineStrong : Ink::gridLine));
    }

    // ── The response, from the engine's own band design ──────────────────────
    NSBezierPath* curve = [NSBezierPath bezierPath];
    NSBezierPath* under = [NSBezierPath bezierPath];

    const CGFloat step = 1.0;

    for (CGFloat x = NSMinX(plot); x <= NSMaxX(plot); x += step) {
        const double t  = (x - NSMinX(plot)) / plot.size.width;
        const double hz = minPlotHz * std::pow(maxPlotHz / minPlotHz, t);
        const double db = std::clamp(dsp::eqMagnitudeDb(_values, _sampleRate, hz),
                                     -displayDb, static_cast<double>(displayDb));

        const NSPoint point = NSMakePoint(x, midY + static_cast<CGFloat>(db) * scale);

        if (x == NSMinX(plot)) {
            [curve moveToPoint:point];
            [under moveToPoint:NSMakePoint(x, midY)];
            [under lineToPoint:point];
        } else {
            [curve lineToPoint:point];
            [under lineToPoint:point];
        }
    }

    [under lineToPoint:NSMakePoint(NSMaxX(plot), midY)];
    [under closePath];

    [theme::withAlpha(theme::ink(Ink::accent), 0.18) setFill];
    [under fill];

    curve.lineWidth = 2.0;
    [theme::ink(Ink::accent) setStroke];
    [curve stroke];
}

- (void)drawBands
{
    for (std::size_t index = 0; index < 3; ++index) {
        const BandSpec& band  = bands[index];
        const double    gain  = _values[band.gainId];
        const double    range = _maxima[band.gainId] - _minima[band.gainId];

        theme::drawTextCentred(@(band.caption),
                               [self rectFromTop:bandLabelTop
                                           height:14.0
                                                x:columnX(index)
                                            width:columnWidth()],
                               theme::ink(Ink::textSecondary),
                               theme::labelFont(10.5, NSFontWeightSemibold),
                               theme::Align::centre);

        const double normalised = range > 0.0
                                    ? (gain - _minima[band.gainId]) / range
                                    : 0.5;

        theme::drawKnob([self knobRectAt:index], normalised,
                        gain == 0.0 ? theme::ink(Ink::accentDim) : theme::ink(Ink::accent),
                        true);

        theme::drawTextCentred(gainText(gain),
                               [self rectFromTop:gainTop
                                           height:16.0
                                                x:columnX(index)
                                            width:columnWidth()],
                               theme::ink(Ink::lcdText),
                               theme::numericFont(12.0), theme::Align::centre);

        theme::drawTextCentred(frequencyText(_values[band.freqId]),
                               [self rectFromTop:freqTop
                                           height:13.0
                                                x:columnX(index)
                                            width:columnWidth()],
                               theme::ink(Ink::textDim),
                               theme::numericFont(9.5, NSFontWeightRegular),
                               theme::Align::centre);
    }
}

- (void)drawSection
{
    theme::drawSeparator([self rectFromTop:sectionTop - 10.0
                                     height:1.0
                                          x:margin
                                      width:panelWidth - margin * 2.0]);

    theme::drawTextCentred(_advancedShown ? @"▾ ADVANCED" : @"▸ ADVANCED",
                           [self advancedToggleRect], theme::ink(Ink::textSecondary),
                           theme::labelFont(10.5, NSFontWeightSemibold));

    const NSRect reset = [self resetRect];
    theme::drawPanel(reset, theme::metrics::radiusControl, false, false);
    theme::drawTextCentred(@"Flat", reset, theme::ink(Ink::textSecondary),
                           theme::labelFont(11.0), theme::Align::centre);
}

- (void)drawAdvanced
{
    for (std::size_t row = 0; row < 4; ++row) {
        const std::uint32_t id  = advancedIds[row];
        const CGFloat       top = advancedTop + advancedRow * static_cast<CGFloat>(row);

        static const char* captions[4] = {"Low Freq", "Mid Freq", "Mid Q", "High Freq"};

        theme::drawTextCentred(@(captions[row]),
                               [self rectFromTop:top
                                           height:advancedRow
                                                x:margin
                                            width:88.0],
                               theme::ink(Ink::textDim), theme::labelFont(10.5));

        theme::drawSlider([self advancedSliderRectAt:row],
                          toNormalised(_values[id], _minima[id], _maxima[id],
                                       isLogarithmic(id)),
                          theme::ink(Ink::accentDim), false);

        NSString* text = id == dsp::EqEffect::midQ
                           ? [NSString stringWithFormat:@"%.2f", _values[id]]
                           : frequencyText(_values[id]);

        theme::drawTextCentred(text,
                               [self rectFromTop:top
                                           height:advancedRow
                                                x:panelWidth - margin - 66.0
                                            width:66.0],
                               theme::ink(Ink::lcdText), theme::numericFont(10.0),
                               theme::Align::right);
    }
}

// ── Editing ──────────────────────────────────────────────────────────────────

- (void)write:(std::uint32_t)parameterId value:(double)value
{
    const double clamped = std::clamp(value, _minima[parameterId], _maxima[parameterId]);
    if (clamped == _values[parameterId])
        return;

    _values[parameterId] = clamped;

    if (_onWrite != nil)
        _onWrite(parameterId, clamped);

    self.needsDisplay = YES;
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];

    _drag = Drag::none;

    if (NSPointInRect(point, [self resetRect])) {
        for (const BandSpec& band : bands)
            [self write:band.gainId value:0.0];

        return;
    }

    if (NSPointInRect(point, [self advancedToggleRect])) {
        _advancedShown = !_advancedShown;
        [self resizeWindowToContent];
        self.needsDisplay = YES;
        return;
    }

    for (std::size_t index = 0; index < 3; ++index) {
        if (!NSPointInRect(point, NSInsetRect([self knobRectAt:index], -6.0, -6.0)))
            continue;

        if (event.clickCount == 2) {
            [self write:bands[index].gainId value:0.0];
            return;
        }

        _drag           = Drag::knob;
        _dragIndex      = index;
        _dragStartValue = _values[bands[index].gainId];
        _dragStartPoint = point;
        return;
    }

    if (!_advancedShown)
        return;

    for (std::size_t row = 0; row < 4; ++row) {
        const NSRect slider = [self advancedSliderRectAt:row];
        if (!NSPointInRect(point, NSInsetRect(slider, 0.0, -6.0)))
            continue;

        _drag      = Drag::advanced;
        _dragIndex = row;
        [self applyAdvancedAt:point row:row];
        return;
    }
}

/// Grows and shrinks the window around the disclosure, anchored at its title
/// bar: a collapsed panel that kept the expanded height would be a third of a
/// window of nothing.
- (void)resizeWindowToContent
{
    NSWindow* window = self.window;
    if (window == nil)
        return;

    const CGFloat wanted = _advancedShown ? expandedHeight : collapsedHeight;
    const CGFloat delta  = wanted - self.bounds.size.height;
    if (delta == 0.0)
        return;

    NSRect frame = window.frame;
    frame.origin.y    -= delta;
    frame.size.height += delta;

    [window setFrame:frame display:YES];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (_drag == Drag::none)
        return;

    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];

    if (_drag == Drag::advanced) {
        [self applyAdvancedAt:point row:_dragIndex];
        return;
    }

    // Mapping the knob's own height to the full ±24 dB would make every pixel
    // worth a third of a decibel. 160 px of travel is what the gesture wants,
    // and ⌥ divides it again when the move has to be a fine one.
    const std::uint32_t id    = bands[_dragIndex].gainId;
    const double        range = _maxima[id] - _minima[id];
    const double        fine  = (event.modifierFlags & NSEventModifierFlagOption) != 0
                                  ? 0.25 : 1.0;

    const double delta = static_cast<double>(point.y - _dragStartPoint.y) / 160.0
                       * range * fine;

    double value = _dragStartValue + delta;
    if (std::abs(value) < detentDb)
        value = 0.0;

    [self write:id value:value];
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;
    _drag = Drag::none;
}

- (void)applyAdvancedAt:(NSPoint)point row:(std::size_t)row
{
    const std::uint32_t id     = advancedIds[row];
    const NSRect        slider = [self advancedSliderRectAt:row];

    const double normalised = slider.size.width > 0.0
                                ? (point.x - NSMinX(slider)) / slider.size.width
                                : 0.0;

    [self write:id
          value:fromNormalised(normalised, _minima[id], _maxima[id], isLogarithmic(id))];
}

@end

// ── The panel ────────────────────────────────────────────────────────────────

@implementation INCDAWTonePanel

+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    rows:(NSArray<NSDictionary*>*)rows
                              sampleRate:(double)sampleRate
                                 onWrite:(void (^)(std::uint32_t, double))onWrite
{
    if (rows.count != dsp::EqEffect::paramCount)
        return nil;

    INCDAWToneView* view =
        [[INCDAWToneView alloc] initWithFrame:NSMakeRect(0, 0, panelWidth, collapsedHeight)];

    // The rows must be exactly the EQ's seven parameters; anything else is a
    // slot this panel has no business editing, and the caller falls back.
    bool seen[dsp::EqEffect::paramCount] = {};

    for (NSDictionary* row in rows) {
        const std::uint32_t id = [row[@"id"] unsignedIntValue];
        if (id >= dsp::EqEffect::paramCount)
            return nil;

        seen[id] = true;

        view->_values[id] = [row[@"value"] doubleValue];
        view->_minima[id] = [row[@"min"] doubleValue];
        view->_maxima[id] = [row[@"max"] doubleValue];
    }

    for (bool present : seen)
        if (!present)
            return nil;

    view->_sampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    view->_onWrite    = onWrite;

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, panelWidth, collapsedHeight)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    window.releasedWhenClosed = NO;
    window.title              = title;
    window.contentView        = view;
    window.appearance         = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    window.backgroundColor    = theme::ink(Ink::windowBackground);

    return window;
}

+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values
{
    if (![window.contentView isKindOfClass:[INCDAWToneView class]])
        return;

    // A drag in progress owns the knobs; the next tick catches up.
    if ((NSEvent.pressedMouseButtons & 1) != 0)
        return;

    INCDAWToneView* view    = static_cast<INCDAWToneView*>(window.contentView);
    BOOL            changed = NO;

    for (std::uint32_t id = 0; id < dsp::EqEffect::paramCount; ++id) {
        NSNumber* incoming = values[@(id)];
        if (incoming == nil || incoming.doubleValue == view->_values[id])
            continue;

        view->_values[id] = incoming.doubleValue;
        changed           = YES;
    }

    if (changed)
        view.needsDisplay = YES;
}

@end
