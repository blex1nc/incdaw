#import "ui/macos/ShaperPanel.h"

#include "engine/dsp/effects/ShaperEffects.h"
#include "ui/macos/Theme.h"

#import <objc/runtime.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace theme = incdaw::ui::theme;
namespace dsp   = incdaw::engine::dsp;
using incdaw::ui::theme::Ink;

namespace {

constexpr CGFloat panelWidth  = 380.0;
constexpr CGFloat plotInset   = 16.0;
constexpr CGFloat plotSize    = 240.0;
constexpr CGFloat rowHeight   = 26.0;
constexpr CGFloat handleRadius = 5.0;

constexpr std::size_t pointCount = dsp::shaperPointCount;

/// The four sliders under the plot, in the order they are shown.
constexpr std::uint32_t sliderIds[] = {
    dsp::WaveshaperEffect::driveDb,
    dsp::WaveshaperEffect::mix,
    dsp::WaveshaperEffect::outputDb,
    dsp::WaveshaperEffect::oversample,
};

constexpr std::size_t sliderCount = std::size(sliderIds);

} // namespace

// ── The plot ─────────────────────────────────────────────────────────────────

@interface INCDAWShaperView : NSView
@end

@implementation INCDAWShaperView {
@public
    std::array<double, pointCount> _points;
    void (^_onWrite)(std::uint32_t, double);
    std::ptrdiff_t _dragging;
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _dragging = -1;
    for (std::size_t index = 0; index < pointCount; ++index)
        _points[index] = dsp::shaperPointX(index);

    return self;
}

- (BOOL)isFlipped
{
    return NO;
}

- (NSRect)plotRect
{
    const CGFloat side = std::min(plotSize, std::min(self.bounds.size.width - plotInset * 2,
                                                     self.bounds.size.height - plotInset * 2));

    return NSMakeRect((self.bounds.size.width - side) * 0.5,
                      self.bounds.size.height - side - plotInset, side, side);
}

- (NSPoint)pointForX:(double)x y:(double)y
{
    const NSRect plot = [self plotRect];

    return NSMakePoint(plot.origin.x + static_cast<CGFloat>((x + 1.0) * 0.5) * plot.size.width,
                       plot.origin.y + static_cast<CGFloat>((y + 1.0) * 0.5) * plot.size.height);
}

- (double)valueForLocation:(NSPoint)location
{
    const NSRect plot = [self plotRect];
    if (plot.size.height <= 0.0)
        return 0.0;

    const double normalised = (location.y - plot.origin.y) / plot.size.height;
    return std::clamp(normalised * 2.0 - 1.0, -1.0, 1.0);
}

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    theme::fillRect(self.bounds, theme::ink(Ink::panel));

    const NSRect plot = [self plotRect];
    theme::fillRect(plot, theme::ink(Ink::panelSunken));

    // The grid, and the identity diagonal it is measured against.
    [theme::ink(Ink::gridLine) set];
    for (int step = 1; step < 4; ++step) {
        const CGFloat fraction = static_cast<CGFloat>(step) / 4.0;
        NSRectFill(NSMakeRect(plot.origin.x + plot.size.width * fraction, plot.origin.y,
                              1.0, plot.size.height));
        NSRectFill(NSMakeRect(plot.origin.x, plot.origin.y + plot.size.height * fraction,
                              plot.size.width, 1.0));
    }

    [theme::ink(Ink::gridLineStrong) set];
    NSRectFill(NSMakeRect(plot.origin.x + plot.size.width * 0.5, plot.origin.y,
                          1.0, plot.size.height));
    NSRectFill(NSMakeRect(plot.origin.x, plot.origin.y + plot.size.height * 0.5,
                          plot.size.width, 1.0));

    NSBezierPath* diagonal = [NSBezierPath bezierPath];
    [diagonal moveToPoint:[self pointForX:-1.0 y:-1.0]];
    [diagonal lineToPoint:[self pointForX:1.0 y:1.0]];
    [diagonal setLineWidth:1.0];
    [[theme::ink(Ink::textDim) colorWithAlphaComponent:0.5] set];
    [diagonal stroke];

    // The curve, from the SAME spline the audio thread builds its table from.
    NSBezierPath* curve = [NSBezierPath bezierPath];

    const int steps = static_cast<int>(plot.size.width);
    for (int step = 0; step <= steps; ++step) {
        const double x = -1.0 + 2.0 * static_cast<double>(step) / static_cast<double>(steps);
        const double y = dsp::shaperCurveAt(_points.data(), x);

        const NSPoint where = [self pointForX:x y:std::clamp(y, -1.0, 1.0)];

        if (step == 0)
            [curve moveToPoint:where];
        else
            [curve lineToPoint:where];
    }

    [curve setLineWidth:2.0];
    [theme::ink(Ink::accent) set];
    [curve stroke];

    // The handles.
    for (std::size_t index = 0; index < pointCount; ++index) {
        const NSPoint where = [self pointForX:dsp::shaperPointX(index) y:_points[index]];

        const NSRect box = NSMakeRect(where.x - handleRadius, where.y - handleRadius,
                                      handleRadius * 2.0, handleRadius * 2.0);

        [(static_cast<std::ptrdiff_t>(index) == _dragging ? theme::ink(Ink::accent)
                                                          : theme::ink(Ink::textPrimary)) set];
        [[NSBezierPath bezierPathWithOvalInRect:box] fill];
    }
}

- (std::ptrdiff_t)handleNear:(NSPoint)location
{
    std::ptrdiff_t best     = -1;
    CGFloat        bestDist = handleRadius * 3.0;

    for (std::size_t index = 0; index < pointCount; ++index) {
        const NSPoint where = [self pointForX:dsp::shaperPointX(index) y:_points[index]];
        const CGFloat distance = std::hypot(where.x - location.x, where.y - location.y);

        if (distance < bestDist) {
            bestDist = distance;
            best     = static_cast<std::ptrdiff_t>(index);
        }
    }

    return best;
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint location = [self convertPoint:event.locationInWindow fromView:nil];

    _dragging = [self handleNear:location];
    if (_dragging < 0)
        return;

    [self mouseDragged:event];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (_dragging < 0)
        return;

    const NSPoint location = [self convertPoint:event.locationInWindow fromView:nil];
    const double  value    = [self valueForLocation:location];

    const auto index = static_cast<std::size_t>(_dragging);
    if (_points[index] == value)
        return;

    _points[index]    = value;
    self.needsDisplay = YES;

    if (_onWrite != nil)
        _onWrite(dsp::WaveshaperEffect::pointBase + static_cast<std::uint32_t>(index), value);
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;
    _dragging = -1;
    self.needsDisplay = YES;
}

/// Double-clicking a handle puts it back on the identity — the way out of a
/// curve that has been dragged somewhere unhelpful.
- (void)rightMouseDown:(NSEvent*)event
{
    const NSPoint        location = [self convertPoint:event.locationInWindow fromView:nil];
    const std::ptrdiff_t handle   = [self handleNear:location];
    if (handle < 0)
        return;

    const auto   index = static_cast<std::size_t>(handle);
    const double value = dsp::shaperPointX(index);

    _points[index]    = value;
    self.needsDisplay = YES;

    if (_onWrite != nil)
        _onWrite(dsp::WaveshaperEffect::pointBase + static_cast<std::uint32_t>(index), value);
}

@end

// ── The panel ────────────────────────────────────────────────────────────────

@implementation INCDAWShaperPanel {
    INCDAWShaperView*      _plot;
    NSArray<NSSlider*>*    _sliders;
    NSArray<NSTextField*>* _labels;
    void (^_onWrite)(std::uint32_t, double);
}

+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    rows:(NSArray<NSDictionary*>*)rows
                                 onWrite:(void (^)(std::uint32_t, double))onWrite
{
    if (rows.count != sliderCount + pointCount)
        return nil;

    INCDAWShaperPanel* panel = [[INCDAWShaperPanel alloc] init];
    panel->_onWrite = onWrite;

    const CGFloat height =
        plotSize + plotInset * 2 + rowHeight * static_cast<CGFloat>(sliderCount) + plotInset;

    INCDAWShaperView* plot =
        [[INCDAWShaperView alloc] initWithFrame:NSMakeRect(0, 0, panelWidth, height)];
    plot.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    plot->_onWrite        = onWrite;

    // The rows must be exactly the shaper's parameters; anything else is a
    // slot this panel has no business editing, and the caller falls back.
    NSMutableDictionary<NSNumber*, NSDictionary*>* byId = [NSMutableDictionary dictionary];
    for (NSDictionary* row in rows)
        byId[row[@"id"]] = row;

    for (std::size_t index = 0; index < pointCount; ++index) {
        NSDictionary* row =
            byId[@(dsp::WaveshaperEffect::pointBase + static_cast<std::uint32_t>(index))];
        if (row == nil)
            return nil;

        plot->_points[index] = [row[@"value"] doubleValue];
    }

    NSMutableArray<NSSlider*>*    sliders = [NSMutableArray array];
    NSMutableArray<NSTextField*>* labels  = [NSMutableArray array];

    for (std::size_t index = 0; index < sliderCount; ++index) {
        NSDictionary* row = byId[@(sliderIds[index])];
        if (row == nil)
            return nil;

        const CGFloat y = plotInset + rowHeight * static_cast<CGFloat>(sliderCount - index - 1);

        NSTextField* label  = [NSTextField labelWithString:row[@"name"]];
        label.frame         = NSMakeRect(plotInset, y + 4, 90, 18);
        label.font          = theme::labelFont(11.0);
        label.textColor     = theme::ink(Ink::textSecondary);
        [plot addSubview:label];
        [labels addObject:label];

        NSSlider* slider = [NSSlider sliderWithValue:[row[@"value"] doubleValue]
                                            minValue:[row[@"min"] doubleValue]
                                            maxValue:[row[@"max"] doubleValue]
                                              target:panel
                                              action:@selector(sliderMoved:)];
        slider.frame      = NSMakeRect(plotInset + 96, y,
                                       panelWidth - plotInset * 2 - 96, rowHeight - 4);
        slider.continuous = YES;
        slider.tag        = static_cast<NSInteger>(index);

        if ([row[@"stepped"] boolValue]) {
            const double span = [row[@"max"] doubleValue] - [row[@"min"] doubleValue];
            slider.numberOfTickMarks        = static_cast<NSInteger>(span) + 1;
            slider.allowsTickMarkValuesOnly = YES;
        }

        [plot addSubview:slider];
        [sliders addObject:slider];
    }

    panel->_plot    = plot;
    panel->_sliders = sliders;
    panel->_labels  = labels;

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, panelWidth, height)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    window.releasedWhenClosed = NO;
    window.title              = title;
    window.contentView        = plot;
    window.appearance         = [NSAppearance appearanceNamed:theme::paletteIsLight()
                                                                  ? NSAppearanceNameAqua
                                                                  : NSAppearanceNameDarkAqua];
    window.backgroundColor    = theme::ink(Ink::windowBackground);

    // The window owns the plot; the plot owns the panel, which owns the
    // slider targets. Without this the panel dies the moment it goes out of
    // scope and the sliders point at nothing.
    objc_setAssociatedObject(window, @selector(makePanelWithTitle:rows:onWrite:), panel,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);

    return window;
}

+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values
{
    if (![window.contentView isKindOfClass:[INCDAWShaperView class]])
        return;

    // A drag in progress owns the handles; the next tick catches up.
    if ((NSEvent.pressedMouseButtons & 1) != 0)
        return;

    INCDAWShaperPanel* panel =
        objc_getAssociatedObject(window, @selector(makePanelWithTitle:rows:onWrite:));
    if (panel == nil)
        return;

    INCDAWShaperView* plot    = panel->_plot;
    BOOL              changed = NO;

    for (std::size_t index = 0; index < pointCount; ++index) {
        NSNumber* incoming =
            values[@(dsp::WaveshaperEffect::pointBase + static_cast<std::uint32_t>(index))];
        if (incoming == nil || incoming.doubleValue == plot->_points[index])
            continue;

        plot->_points[index] = incoming.doubleValue;
        changed              = YES;
    }

    for (std::size_t index = 0; index < sliderCount; ++index) {
        NSNumber* incoming = values[@(sliderIds[index])];
        if (incoming == nil || index >= panel->_sliders.count)
            continue;

        if (panel->_sliders[index].doubleValue != incoming.doubleValue)
            panel->_sliders[index].doubleValue = incoming.doubleValue;
    }

    if (changed)
        plot.needsDisplay = YES;
}

+ (void)refreshAppearance:(NSWindow*)window
{
    if (![window.contentView isKindOfClass:[INCDAWShaperView class]])
        return;

    INCDAWShaperPanel* panel =
        objc_getAssociatedObject(window, @selector(makePanelWithTitle:rows:onWrite:));
    if (panel == nil)
        return;

    for (NSTextField* label in panel->_labels)
        label.textColor = theme::ink(Ink::textSecondary);

    panel->_plot.needsDisplay = YES;
}

- (void)sliderMoved:(NSSlider*)slider
{
    const auto index = static_cast<std::size_t>(slider.tag);
    if (index >= sliderCount || _onWrite == nil)
        return;

    _onWrite(sliderIds[index], slider.doubleValue);
}

@end
