// The Beat Gate's editor: the two curves, drawn over one bar.
//
// Thirty-two numbered sliders is not an editor for a gesture that is drawn.
// This is two stacked plots on a sixteenth-note grid — time above, volume
// below — where a gesture is swept in with the mouse the way it is meant to
// be. Both are plotted from engine::dsp::beatGateCurveAt, the same
// interpolation the audio thread reads.

#pragma once

#import <Cocoa/Cocoa.h>

#include <cstdint>

NS_ASSUME_NONNULL_BEGIN

@interface INCDAWBeatGatePanel : NSObject

/// Builds the window. `rows` are the shell's parameter rows for a
/// BeatGateEffect slot; a row set that does not carry its ids is rejected
/// with nil. `onWrite` receives PLAIN values.
+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    rows:(NSArray<NSDictionary*>*)rows
                                 onWrite:(void (^)(std::uint32_t parameterId,
                                                   double plainValue))onWrite;

/// Moves the curves and sliders to `values`. Ignored while the mouse is down.
+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values;

/// Re-reads the palette into the panel's AppKit controls.
+ (void)refreshAppearance:(NSWindow*)window;

@end

NS_ASSUME_NONNULL_END
