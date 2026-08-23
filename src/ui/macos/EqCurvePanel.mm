#import "ui/macos/EqCurvePanel.h"

#include "engine/dsp/effects/ParametricEq.h"
#include "ui/macos/Theme.h"

#import <objc/runtime.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace theme = incdaw::ui::theme;
namespace dsp   = incdaw::engine::dsp;
using incdaw::ui::theme::Ink;

namespace {

constexpr CGFloat panelWidth  = 520.0;
constexpr CGFloat plotHeight  = 260.0;
constexpr CGFloat margin      = 12.0;
constexpr CGFloat footerHeight = 30.0;

constexpr double lowestHz  = 20.0;
constexpr double highestHz = 20000.0;
constexpr double rangeDb   = 24.0;

constexpr std::size_t bandCount = dsp::parametricBandCount;

using Eq = dsp::ParametricEqEffect;

/// Frequency to a 0..1 position, logarithmically — the only axis on which an
/// equaliser is readable.
double positionForFrequency(double frequency)
{
    const double clamped = std::clamp(frequency, lowestHz, highestHz);
    return std::log(clamped / lowestHz) / std::log(highestHz / lowestHz);
}

double frequencyForPosition(double position)
{
    const double clamped = std::clamp(position, 0.0, 1.0);
    return lowestHz * std::pow(highestHz / lowestHz, clamped);
}

} // namespace

@interface INCDAWEqCurveView : NSView
@end

@implementation INCDAWEqCurveView {
@public
    std::array<dsp::ParametricBand, bandCount> _bands;
    double _sampleRate;
    void (^_onWrite)(std::uint32_t, double);
    std::ptrdiff_t _dragging;
    std::ptrdiff_t _hovered;
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _sampleRate = 48000.0;
    _dragging   = -1;
    _hovered    = -1;
    return self;
}

- (BOOL)isFlipped
{
    return NO;
}

- (NSRect)plotRect
{
    return NSMakeRect(margin, footerHeight, self.bounds.size.width - margin * 2,
                      self.bounds.size.height - footerHeight - margin);
}

- (NSPoint)pointForFrequency:(double)frequency gain:(double)gainDb
{
    const NSRect plot = [self plotRect];

    const CGFloat x = plot.origin.x
                    + static_cast<CGFloat>(positionForFrequency(frequency)) * plot.size.width;
    const CGFloat y = plot.origin.y
                    + static_cast<CGFloat>((std::clamp(gainDb, -rangeDb, rangeDb) + rangeDb)
                                           / (rangeDb * 2.0))
                          * plot.size.height;

    return NSMakePoint(x, y);
}

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    theme::fillRect(self.bounds, theme::ink(Ink::panel));

    const NSRect plot = [self plotRect];
    theme::fillRect(plot, theme::ink(Ink::panelSunken));

    // Decade lines, and the 0 dB line they hang from.
    [theme::ink(Ink::gridLine) set];
    for (const double frequency : {100.0, 1000.0, 10000.0}) {
        const CGFloat x = plot.origin.x
                        + static_cast<CGFloat>(positionForFrequency(frequency))
                              * plot.size.width;
        NSRectFill(NSMakeRect(x, plot.origin.y, 1.0, plot.size.height));
    }

    for (const double gain : {-12.0, 12.0}) {
        const CGFloat y = [self pointForFrequency:1000.0 gain:gain].y;
        NSRectFill(NSMakeRect(plot.origin.x, y, plot.size.width, 1.0));
    }

    [theme::ink(Ink::gridLineStrong) set];
    NSRectFill(NSMakeRect(plot.origin.x, [self pointForFrequency:1000.0 gain:0.0].y,
                          plot.size.width, 1.0));

    // The curve, from the same design the audio thread runs.
    NSBezierPath* curve = [NSBezierPath bezierPath];

    const int steps = static_cast<int>(plot.size.width);
    for (int step = 0; step <= steps; ++step) {
        const double position  = static_cast<double>(step) / static_cast<double>(steps);
        const double frequency = frequencyForPosition(position);
        const double gain = dsp::parametricMagnitudeDb(_bands, _sampleRate, frequency);

        const NSPoint where = [self pointForFrequency:frequency gain:gain];

        if (step == 0)
            [curve moveToPoint:where];
        else
            [curve lineToPoint:where];
    }

    [curve setLineWidth:2.0];
    [theme::ink(Ink::accent) set];
    [curve stroke];

    // A handle per band that is doing something.
    for (std::size_t index = 0; index < bandCount; ++index) {
        if (_bands[index].type == dsp::ParametricBandType::off)
            continue;

        const NSPoint where = [self pointForFrequency:_bands[index].frequency
                                                 gain:_bands[index].gainDb];

        const CGFloat radius = static_cast<std::ptrdiff_t>(index) == _dragging ? 7.0 : 5.0;
        const NSRect  box    = NSMakeRect(where.x - radius, where.y - radius,
                                          radius * 2.0, radius * 2.0);

        [(static_cast<std::ptrdiff_t>(index) == _dragging ? theme::ink(Ink::accent)
                                                          : theme::ink(Ink::textPrimary)) set];
        [[NSBezierPath bezierPathWithOvalInRect:box] fill];

        NSString* label = [NSString stringWithFormat:@"%zu", index + 1];
        [label drawAtPoint:NSMakePoint(where.x + radius + 2.0, where.y - 6.0)
            withAttributes:@{
                NSFontAttributeName: theme::numericFont(9.0, NSFontWeightRegular),
                NSForegroundColorAttributeName: theme::ink(Ink::textDim),
            }];
    }

    // The footer says what the grabbed band is, in words.
    NSString* caption = @"Drag a band · scroll for Q · right-click for its type";

    if (_dragging >= 0) {
        const auto index = static_cast<std::size_t>(_dragging);
        caption = [NSString stringWithFormat:@"Band %zu · %s · %.0f Hz · %+.1f dB · Q %.2f",
                                             index + 1,
                                             dsp::parametricBandTypeName(_bands[index].type),
                                             _bands[index].frequency, _bands[index].gainDb,
                                             _bands[index].q];
    }

    [caption drawAtPoint:NSMakePoint(margin, 8.0)
          withAttributes:@{
              NSFontAttributeName: theme::labelFont(11.0),
              NSForegroundColorAttributeName: theme::ink(Ink::textSecondary),
          }];
}

- (std::ptrdiff_t)bandNear:(NSPoint)location
{
    std::ptrdiff_t best     = -1;
    CGFloat        bestDist = 18.0;

    for (std::size_t index = 0; index < bandCount; ++index) {
        if (_bands[index].type == dsp::ParametricBandType::off)
            continue;

        const NSPoint where = [self pointForFrequency:_bands[index].frequency
                                                 gain:_bands[index].gainDb];
        const CGFloat distance = std::hypot(where.x - location.x, where.y - location.y);

        if (distance < bestDist) {
            bestDist = distance;
            best     = static_cast<std::ptrdiff_t>(index);
        }
    }

    return best;
}

- (void)write:(std::size_t)band offset:(Eq::BandOffset)offset value:(double)value
{
    if (_onWrite != nil)
        _onWrite(Eq::bandParameter(band, offset), value);
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint location = [self convertPoint:event.locationInWindow fromView:nil];

    _dragging = [self bandNear:location];

    // Clicking empty space switches the nearest OFF band on, where it was
    // clicked — the fastest way to add a band there is to point at it.
    if (_dragging < 0) {
        for (std::size_t index = 0; index < bandCount; ++index) {
            if (_bands[index].type != dsp::ParametricBandType::off)
                continue;

            const NSRect plot = [self plotRect];
            const double position =
                (location.x - plot.origin.x) / std::max(plot.size.width, 1.0);
            const double gain =
                ((location.y - plot.origin.y) / std::max(plot.size.height, 1.0)) * rangeDb * 2.0
                - rangeDb;

            _bands[index].type      = dsp::ParametricBandType::peak;
            _bands[index].frequency = frequencyForPosition(position);
            _bands[index].gainDb    = std::clamp(gain, -rangeDb, rangeDb);

            [self write:index offset:Eq::bandType
                  value:static_cast<double>(dsp::ParametricBandType::peak)];
            [self write:index offset:Eq::bandFrequency value:_bands[index].frequency];
            [self write:index offset:Eq::bandGainDb value:_bands[index].gainDb];

            _dragging         = static_cast<std::ptrdiff_t>(index);
            self.needsDisplay = YES;
            return;
        }

        return;
    }

    self.needsDisplay = YES;
}

- (void)mouseDragged:(NSEvent*)event
{
    if (_dragging < 0)
        return;

    const NSPoint location = [self convertPoint:event.locationInWindow fromView:nil];
    const NSRect  plot     = [self plotRect];

    const auto index = static_cast<std::size_t>(_dragging);

    const double position = (location.x - plot.origin.x) / std::max(plot.size.width, 1.0);
    const double gain =
        ((location.y - plot.origin.y) / std::max(plot.size.height, 1.0)) * rangeDb * 2.0
        - rangeDb;

    _bands[index].frequency = frequencyForPosition(position);
    _bands[index].gainDb    = std::clamp(gain, -rangeDb, rangeDb);

    [self write:index offset:Eq::bandFrequency value:_bands[index].frequency];
    [self write:index offset:Eq::bandGainDb value:_bands[index].gainDb];

    self.needsDisplay = YES;
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;
    _dragging         = -1;
    self.needsDisplay = YES;
}

- (void)scrollWheel:(NSEvent*)event
{
    const NSPoint        location = [self convertPoint:event.locationInWindow fromView:nil];
    const std::ptrdiff_t band     = [self bandNear:location];
    if (band < 0)
        return;

    const auto   index = static_cast<std::size_t>(band);
    const double step  = 1.0 + static_cast<double>(event.scrollingDeltaY) * 0.02;

    _bands[index].q = std::clamp(_bands[index].q * step, 0.1, 18.0);

    [self write:index offset:Eq::bandQ value:_bands[index].q];
    self.needsDisplay = YES;
}

/// Right-click walks the band through its types, ending at Off — which is
/// also how a band is removed.
- (void)rightMouseDown:(NSEvent*)event
{
    const NSPoint        location = [self convertPoint:event.locationInWindow fromView:nil];
    const std::ptrdiff_t band     = [self bandNear:location];
    if (band < 0)
        return;

    const auto index = static_cast<std::size_t>(band);
    const int  next  = (static_cast<int>(_bands[index].type) + 1) % dsp::parametricBandTypeCount;

    _bands[index].type = static_cast<dsp::ParametricBandType>(next);

    [self write:index offset:Eq::bandType value:static_cast<double>(next)];
    self.needsDisplay = YES;
}

@end

@implementation INCDAWEqCurvePanel {
    INCDAWEqCurveView* _view;
}

+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    rows:(NSArray<NSDictionary*>*)rows
                              sampleRate:(double)sampleRate
                                 onWrite:(void (^)(std::uint32_t, double))onWrite
{
    if (rows.count != 1 + bandCount * 4)
        return nil;

    NSMutableDictionary<NSNumber*, NSDictionary*>* byId = [NSMutableDictionary dictionary];
    for (NSDictionary* row in rows)
        byId[row[@"id"]] = row;

    INCDAWEqCurveView* view = [[INCDAWEqCurveView alloc]
        initWithFrame:NSMakeRect(0, 0, panelWidth, plotHeight + footerHeight + margin)];
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    for (std::size_t band = 0; band < bandCount; ++band) {
        NSDictionary* type      = byId[@(Eq::bandParameter(band, Eq::bandType))];
        NSDictionary* frequency = byId[@(Eq::bandParameter(band, Eq::bandFrequency))];
        NSDictionary* gain      = byId[@(Eq::bandParameter(band, Eq::bandGainDb))];
        NSDictionary* q         = byId[@(Eq::bandParameter(band, Eq::bandQ))];

        if (type == nil || frequency == nil || gain == nil || q == nil)
            return nil;

        view->_bands[band].type = static_cast<dsp::ParametricBandType>(
            std::clamp([type[@"value"] intValue], 0, dsp::parametricBandTypeCount - 1));
        view->_bands[band].frequency = [frequency[@"value"] doubleValue];
        view->_bands[band].gainDb    = [gain[@"value"] doubleValue];
        view->_bands[band].q         = [q[@"value"] doubleValue];
    }

    view->_sampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    view->_onWrite    = onWrite;

    INCDAWEqCurvePanel* panel = [[INCDAWEqCurvePanel alloc] init];
    panel->_view = view;

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, panelWidth, plotHeight + footerHeight + margin)
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

    objc_setAssociatedObject(window, @selector(makePanelWithTitle:rows:sampleRate:onWrite:),
                             panel, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return window;
}

+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values
{
    if (![window.contentView isKindOfClass:[INCDAWEqCurveView class]])
        return;

    // A drag in progress owns the handles; the next tick catches up.
    if ((NSEvent.pressedMouseButtons & 1) != 0)
        return;

    INCDAWEqCurveView* view    = static_cast<INCDAWEqCurveView*>(window.contentView);
    BOOL               changed = NO;

    for (std::size_t band = 0; band < bandCount; ++band) {
        NSNumber* type      = values[@(Eq::bandParameter(band, Eq::bandType))];
        NSNumber* frequency = values[@(Eq::bandParameter(band, Eq::bandFrequency))];
        NSNumber* gain      = values[@(Eq::bandParameter(band, Eq::bandGainDb))];
        NSNumber* q         = values[@(Eq::bandParameter(band, Eq::bandQ))];

        if (type != nil
            && static_cast<int>(view->_bands[band].type) != type.intValue) {
            view->_bands[band].type = static_cast<dsp::ParametricBandType>(
                std::clamp(type.intValue, 0, dsp::parametricBandTypeCount - 1));
            changed = YES;
        }

        if (frequency != nil && view->_bands[band].frequency != frequency.doubleValue) {
            view->_bands[band].frequency = frequency.doubleValue;
            changed                      = YES;
        }

        if (gain != nil && view->_bands[band].gainDb != gain.doubleValue) {
            view->_bands[band].gainDb = gain.doubleValue;
            changed                   = YES;
        }

        if (q != nil && view->_bands[band].q != q.doubleValue) {
            view->_bands[band].q = q.doubleValue;
            changed              = YES;
        }
    }

    if (changed)
        view.needsDisplay = YES;
}

+ (void)refreshAppearance:(NSWindow*)window
{
    if ([window.contentView isKindOfClass:[INCDAWEqCurveView class]])
        window.contentView.needsDisplay = YES;
}

@end
