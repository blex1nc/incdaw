#import "ui/macos/BeatGatePanel.h"

#include "engine/dsp/effects/BeatGate.h"
#include "ui/macos/Theme.h"

#import <objc/runtime.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace theme = incdaw::ui::theme;
namespace dsp   = incdaw::engine::dsp;
using incdaw::ui::theme::Ink;

namespace {

constexpr CGFloat panelWidth  = 480.0;
constexpr CGFloat plotHeight  = 130.0;
constexpr CGFloat margin      = 12.0;
constexpr CGFloat rowHeight   = 26.0;

constexpr std::size_t pointCount = dsp::beatGatePoints;

using Gate = dsp::BeatGateEffect;

/// The sliders under the plots, in the order they are shown.
constexpr std::uint32_t sliderIds[] = {
    Gate::mix, Gate::timeAmount, Gate::volumeAmount, Gate::smoothingMs, Gate::bars,
};

constexpr std::size_t sliderCount = std::size(sliderIds);

const void* panelOwnerKey = &panelOwnerKey;

} // namespace

@interface INCDAWBeatGateView : NSView
@end

@implementation INCDAWBeatGateView {
@public
    std::array<double, pointCount> _time;
    std::array<double, pointCount> _volume;
    void (^_onWrite)(std::uint32_t, double);
    BOOL _draggingVolume;
    BOOL _dragging;
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _time.fill(0.0);
    _volume.fill(1.0);
    return self;
}

- (BOOL)isFlipped
{
    return NO;
}

/// The upper plot is TIME, the lower is VOLUME.
- (NSRect)plotRect:(BOOL)volume
{
    const CGFloat top = self.bounds.size.height - margin;
    const CGFloat y   = volume ? top - plotHeight * 2 - margin : top - plotHeight;

    return NSMakeRect(margin, y, self.bounds.size.width - margin * 2, plotHeight);
}

- (void)drawCurve:(const std::array<double, pointCount>&)points
             into:(NSRect)plot
            title:(NSString*)title
{
    theme::fillRect(plot, theme::ink(Ink::panelSunken));

    // The sixteenth grid, with the beats picked out.
    for (std::size_t step = 1; step < pointCount; ++step) {
        const CGFloat x = plot.origin.x
                        + plot.size.width * static_cast<CGFloat>(step)
                              / static_cast<CGFloat>(pointCount);

        [((step % 4) == 0 ? theme::ink(Ink::gridLineStrong) : theme::ink(Ink::gridLine)) set];
        NSRectFill(NSMakeRect(x, plot.origin.y, 1.0, plot.size.height));
    }

    NSBezierPath* curve = [NSBezierPath bezierPath];

    const int steps = static_cast<int>(plot.size.width);
    for (int step = 0; step <= steps; ++step) {
        const double phase = static_cast<double>(step) / static_cast<double>(steps);
        const double value = std::clamp(dsp::beatGateCurveAt(points.data(), phase), 0.0, 1.0);

        const NSPoint where =
            NSMakePoint(plot.origin.x + plot.size.width * static_cast<CGFloat>(phase),
                        plot.origin.y + plot.size.height * static_cast<CGFloat>(value));

        if (step == 0)
            [curve moveToPoint:where];
        else
            [curve lineToPoint:where];
    }

    [curve setLineWidth:2.0];
    [theme::ink(Ink::accent) set];
    [curve stroke];

    [title drawAtPoint:NSMakePoint(plot.origin.x + 4.0,
                                   plot.origin.y + plot.size.height - 16.0)
        withAttributes:@{
            NSFontAttributeName: theme::labelFont(10.0),
            NSForegroundColorAttributeName: theme::ink(Ink::textDim),
        }];
}

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    theme::fillRect(self.bounds, theme::ink(Ink::panel));

    [self drawCurve:_time into:[self plotRect:NO] title:@"TIME — how far back to read"];
    [self drawCurve:_volume into:[self plotRect:YES] title:@"VOLUME"];
}

/// Sweeping across the plot writes every point the pointer passes, which is
/// how a gesture gets drawn rather than dialled.
- (void)paintAt:(NSPoint)location
{
    const BOOL   volume = NSPointInRect(location, [self plotRect:YES]);
    const NSRect plot   = [self plotRect:volume];

    if (!NSPointInRect(location, plot) && !_dragging)
        return;

    const double x = (location.x - plot.origin.x) / std::max(plot.size.width, 1.0);
    const double y = (location.y - plot.origin.y) / std::max(plot.size.height, 1.0);

    const auto index = static_cast<std::size_t>(
        std::clamp(x, 0.0, 0.9999) * static_cast<double>(pointCount));

    const double value = std::clamp(y, 0.0, 1.0);

    std::array<double, pointCount>& points = volume ? _volume : _time;
    if (points[index] == value)
        return;

    points[index] = value;

    if (_onWrite != nil)
        _onWrite((volume ? Gate::volumeBase : Gate::timeBase)
                     + static_cast<std::uint32_t>(index),
                 value);

    self.needsDisplay = YES;
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint location = [self convertPoint:event.locationInWindow fromView:nil];

    _draggingVolume = NSPointInRect(location, [self plotRect:YES]);
    _dragging       = NSPointInRect(location, [self plotRect:NO]) || _draggingVolume;

    if (_dragging)
        [self paintAt:location];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (!_dragging)
        return;

    // A drag stays in the plot it started in, so sweeping past the boundary
    // does not start rewriting the other curve.
    NSPoint      location = [self convertPoint:event.locationInWindow fromView:nil];
    const NSRect plot     = [self plotRect:_draggingVolume];

    location.y = std::clamp(location.y, plot.origin.y, plot.origin.y + plot.size.height);
    location.x = std::clamp(location.x, plot.origin.x + 1.0,
                            plot.origin.x + plot.size.width - 1.0);

    [self paintAt:location];
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;
    _dragging = NO;
}

/// Right-click flattens the curve under the pointer — the way out of a
/// gesture that has gone wrong.
- (void)rightMouseDown:(NSEvent*)event
{
    const NSPoint location = [self convertPoint:event.locationInWindow fromView:nil];
    const BOOL    volume   = NSPointInRect(location, [self plotRect:YES]);

    if (!volume && !NSPointInRect(location, [self plotRect:NO]))
        return;

    std::array<double, pointCount>& points = volume ? _volume : _time;
    const double rest = volume ? 1.0 : 0.0;

    for (std::size_t index = 0; index < pointCount; ++index) {
        if (points[index] == rest)
            continue;

        points[index] = rest;

        if (_onWrite != nil)
            _onWrite((volume ? Gate::volumeBase : Gate::timeBase)
                         + static_cast<std::uint32_t>(index),
                     rest);
    }

    self.needsDisplay = YES;
}

@end

@implementation INCDAWBeatGatePanel {
    INCDAWBeatGateView*     _view;
    NSArray<NSSlider*>*     _sliders;
    NSArray<NSTextField*>*  _labels;
    void (^_onWrite)(std::uint32_t, double);
}

+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    rows:(NSArray<NSDictionary*>*)rows
                                 onWrite:(void (^)(std::uint32_t, double))onWrite
{
    if (rows.count != sliderCount + pointCount * 2)
        return nil;

    NSMutableDictionary<NSNumber*, NSDictionary*>* byId = [NSMutableDictionary dictionary];
    for (NSDictionary* row in rows)
        byId[row[@"id"]] = row;

    const CGFloat height = margin * 3 + plotHeight * 2
                         + rowHeight * static_cast<CGFloat>(sliderCount) + margin;

    INCDAWBeatGateView* view =
        [[INCDAWBeatGateView alloc] initWithFrame:NSMakeRect(0, 0, panelWidth, height)];
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    view->_onWrite        = onWrite;

    for (std::size_t index = 0; index < pointCount; ++index) {
        NSDictionary* time   = byId[@(Gate::timeBase + static_cast<std::uint32_t>(index))];
        NSDictionary* volume = byId[@(Gate::volumeBase + static_cast<std::uint32_t>(index))];

        if (time == nil || volume == nil)
            return nil;

        view->_time[index]   = [time[@"value"] doubleValue];
        view->_volume[index] = [volume[@"value"] doubleValue];
    }

    INCDAWBeatGatePanel* panel = [[INCDAWBeatGatePanel alloc] init];
    panel->_view    = view;
    panel->_onWrite = onWrite;

    NSMutableArray<NSSlider*>*    sliders = [NSMutableArray array];
    NSMutableArray<NSTextField*>* labels  = [NSMutableArray array];

    for (std::size_t index = 0; index < sliderCount; ++index) {
        NSDictionary* row = byId[@(sliderIds[index])];
        if (row == nil)
            return nil;

        const CGFloat y = margin + rowHeight * static_cast<CGFloat>(sliderCount - index - 1);

        NSTextField* label = [NSTextField labelWithString:row[@"name"]];
        label.frame        = NSMakeRect(margin, y + 4, 84, 18);
        label.font         = theme::labelFont(11.0);
        label.textColor    = theme::ink(Ink::textSecondary);
        [view addSubview:label];
        [labels addObject:label];

        NSSlider* slider = [NSSlider sliderWithValue:[row[@"value"] doubleValue]
                                            minValue:[row[@"min"] doubleValue]
                                            maxValue:[row[@"max"] doubleValue]
                                              target:panel
                                              action:@selector(sliderMoved:)];
        slider.frame      = NSMakeRect(margin + 90, y, panelWidth - margin * 2 - 90,
                                       rowHeight - 4);
        slider.continuous = YES;
        slider.tag        = static_cast<NSInteger>(index);

        if ([row[@"stepped"] boolValue]) {
            const double span = [row[@"max"] doubleValue] - [row[@"min"] doubleValue];
            slider.numberOfTickMarks        = static_cast<NSInteger>(span) + 1;
            slider.allowsTickMarkValuesOnly = YES;
        }

        [view addSubview:slider];
        [sliders addObject:slider];
    }

    panel->_sliders = sliders;
    panel->_labels  = labels;

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, panelWidth, height)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                          | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    window.releasedWhenClosed = NO;
    window.title              = title;
    window.contentView        = view;
    window.appearance         = [NSAppearance appearanceNamed:theme::paletteIsLight()
                                                                  ? NSAppearanceNameAqua
                                                                  : NSAppearanceNameDarkAqua];
    window.backgroundColor    = theme::ink(Ink::windowBackground);

    objc_setAssociatedObject(window, panelOwnerKey, panel, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return window;
}

+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values
{
    INCDAWBeatGatePanel* panel = objc_getAssociatedObject(window, panelOwnerKey);
    if (panel == nil)
        return;

    // A drag in progress owns the curves; the next tick catches up.
    if ((NSEvent.pressedMouseButtons & 1) != 0)
        return;

    INCDAWBeatGateView* view    = panel->_view;
    BOOL                changed = NO;

    for (std::size_t index = 0; index < pointCount; ++index) {
        NSNumber* time   = values[@(Gate::timeBase + static_cast<std::uint32_t>(index))];
        NSNumber* volume = values[@(Gate::volumeBase + static_cast<std::uint32_t>(index))];

        if (time != nil && view->_time[index] != time.doubleValue) {
            view->_time[index] = time.doubleValue;
            changed            = YES;
        }

        if (volume != nil && view->_volume[index] != volume.doubleValue) {
            view->_volume[index] = volume.doubleValue;
            changed              = YES;
        }
    }

    for (std::size_t index = 0; index < sliderCount && index < panel->_sliders.count; ++index) {
        NSNumber* incoming = values[@(sliderIds[index])];
        if (incoming != nil && panel->_sliders[index].doubleValue != incoming.doubleValue)
            panel->_sliders[index].doubleValue = incoming.doubleValue;
    }

    if (changed)
        view.needsDisplay = YES;
}

+ (void)refreshAppearance:(NSWindow*)window
{
    INCDAWBeatGatePanel* panel = objc_getAssociatedObject(window, panelOwnerKey);
    if (panel == nil)
        return;

    for (NSTextField* label in panel->_labels)
        label.textColor = theme::ink(Ink::textSecondary);

    panel->_view.needsDisplay = YES;
}

- (void)sliderMoved:(NSSlider*)slider
{
    const auto index = static_cast<std::size_t>(slider.tag);
    if (index >= sliderCount || _onWrite == nil)
        return;

    _onWrite(sliderIds[index], slider.doubleValue);
}

@end
