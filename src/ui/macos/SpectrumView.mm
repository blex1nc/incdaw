#import "ui/macos/SpectrumView.h"

#include "ui/macos/Theme.h"

#include <algorithm>
#include <cmath>

namespace theme = incdaw::ui::theme;
using incdaw::ui::theme::Ink;

namespace {

constexpr double minimumHz = 20.0;
constexpr double floorDb   = -90.0;
constexpr double ceilDb    = 0.0;

} // namespace

@implementation INCDAWSpectrumView {
    std::vector<float> _binsDb;
    double             _sampleRate;
}

- (instancetype)initWithFrame:(NSRect)frame
{
    if ((self = [super initWithFrame:frame]) != nil)
        _sampleRate = 48000.0;

    return self;
}

- (void)updateWithBins:(const std::vector<float>&)binsDb sampleRate:(double)sampleRate
{
    _binsDb     = binsDb;
    _sampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    [self setNeedsDisplay:YES];
}

- (BOOL)isOpaque
{
    return YES;
}

- (void)drawRect:(NSRect)dirty
{
    (void)dirty;

    // An analyser is a well with a trace in it, like every other readout in
    // the window.
    theme::drawWell(self.bounds, theme::metrics::radiusPad);

    const NSRect bounds = self.bounds;

    // Decade gridlines: 100 Hz, 1 kHz, 10 kHz.
    const double nyquist = _sampleRate / 2.0;
    const double logLow  = std::log10(minimumHz);
    const double logHigh = std::log10(nyquist);

    [theme::ink(Ink::gridLine) setStroke];
    for (const double hz : {100.0, 1000.0, 10000.0}) {
        if (hz >= nyquist)
            continue;

        const CGFloat x = bounds.size.width
                        * static_cast<CGFloat>((std::log10(hz) - logLow) / (logHigh - logLow));

        NSBezierPath* line = [NSBezierPath bezierPath];
        [line moveToPoint:NSMakePoint(x, 0)];
        [line lineToPoint:NSMakePoint(x, bounds.size.height)];
        line.lineWidth = 1.0;
        [line stroke];
    }

    if (_binsDb.size() < 2)
        return;

    NSBezierPath* path = [NSBezierPath bezierPath];
    bool started = false;

    const std::size_t binCount = _binsDb.size();

    for (std::size_t bin = 1; bin < binCount; ++bin) {
        const double hz = nyquist * static_cast<double>(bin)
                        / static_cast<double>(binCount - 1);
        if (hz < minimumHz)
            continue;

        const double x01 = (std::log10(hz) - logLow) / (logHigh - logLow);
        const double y01 = (std::clamp(static_cast<double>(_binsDb[bin]), floorDb, ceilDb)
                            - floorDb)
                         / (ceilDb - floorDb);

        const NSPoint point = NSMakePoint(bounds.size.width * static_cast<CGFloat>(x01),
                                          bounds.size.height * static_cast<CGFloat>(y01));

        if (!started) {
            [path moveToPoint:point];
            started = true;
        } else {
            [path lineToPoint:point];
        }
    }

    [theme::ink(Ink::accent) setStroke];
    path.lineWidth = 1.5;
    [path stroke];
}

@end
