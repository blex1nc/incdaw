// The Parametric EQ's editor: the response curve, with the bands on it.
//
// Eight bands is thirty-two sliders, and nobody has ever equalised anything
// by dragging thirty-two sliders. The curve is the control: a handle per
// band, dragged left and right for frequency and up and down for gain, the
// scroll wheel for Q, and a right-click to change what the band does.
//
// Like the Tone panel this is deliberately dumb — row DATA and a write block,
// never an engine pointer — and like it, the curve is not a second opinion
// about the filter: it is plotted from engine::dsp::parametricMagnitudeDb,
// the same design the audio thread runs.

#pragma once

#import <Cocoa/Cocoa.h>

#include <cstdint>

NS_ASSUME_NONNULL_BEGIN

@interface INCDAWEqCurvePanel : NSObject

/// Builds the window. `rows` are the shell's parameter rows for a
/// ParametricEqEffect slot; a row set that does not carry its ids is rejected
/// with nil, so the caller falls back to the generic panel. `sampleRate` is
/// used only to plot. `onWrite` receives PLAIN values.
+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    rows:(NSArray<NSDictionary*>*)rows
                              sampleRate:(double)sampleRate
                                 onWrite:(void (^)(std::uint32_t parameterId,
                                                   double plainValue))onWrite;

/// Moves the handles to `values` — how an open panel follows automation,
/// MIDI knobs and undo. Ignored while the mouse is down.
+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values;

/// Re-reads the palette into the panel's AppKit controls.
+ (void)refreshAppearance:(NSWindow*)window;

@end

NS_ASSUME_NONNULL_END
