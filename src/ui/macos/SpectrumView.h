// The analyzer's spectrum display: magnitude per FFT bin on a log-frequency
// axis. Dumb by the shell's usual rule — it owns pixels and a copy of the
// last bins handed to it, never an engine pointer; the shell's housekeeping
// pushes fresh bins while the window is open.

#pragma once

#import <Cocoa/Cocoa.h>

#include <vector>

NS_ASSUME_NONNULL_BEGIN

@interface INCDAWSpectrumView : NSView

- (void)updateWithBins:(const std::vector<float>&)binsDb sampleRate:(double)sampleRate;

@end

NS_ASSUME_NONNULL_END
