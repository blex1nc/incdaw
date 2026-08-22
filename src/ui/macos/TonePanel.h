// The Tone insert's editor: bass, mid and treble as three bipolar knobs over
// a live response curve, with the EQ's frequencies and Q kept out of the way
// until asked for.
//
// It edits the same seven parameters `incdaw.eq` carries — the Tone effect IS
// the three-band EQ — so it takes the shell's ordinary parameter rows and
// writes plain values back through the same block the generic panel uses.
// Like that panel it is deliberately dumb: row DATA and a write block, never
// an engine pointer, because sinks die with their graph on every rebuild.
//
// The curve is not a second opinion about the filter: it is plotted from
// engine::dsp::eqMagnitudeDb, which designs its biquads with the code the
// audio thread runs.

#pragma once

#import <Cocoa/Cocoa.h>

#include <cstdint>

NS_ASSUME_NONNULL_BEGIN

@interface INCDAWTonePanel : NSObject

/// Builds the window. `rows` are the shell's parameter rows for an EqEffect
/// slot (keys: @"id", @"name", @"min", @"max", @"value", @"stepped"); a row
/// set that does not carry the EQ's seven ids is rejected with nil, so the
/// caller can fall back to the generic panel. `sampleRate` is the device's,
/// used only to plot the curve. `onWrite` receives PLAIN values.
+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    rows:(NSArray<NSDictionary*>*)rows
                              sampleRate:(double)sampleRate
                                 onWrite:(void (^)(std::uint32_t parameterId,
                                                   double plainValue))onWrite;

/// Moves the knobs of a Tone panel to `values` (parameter id → plain value) —
/// how an open panel follows automation, MIDI knobs and undo. Ignored while
/// the mouse is down; a window that is not a Tone panel is ignored entirely.
+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values;

@end

NS_ASSUME_NONNULL_END
