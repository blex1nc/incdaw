// The Waveshaper insert's editor: the transfer curve, drawn and dragged.
//
// A shaper whose curve is nine numbered sliders is a shaper nobody will use.
// The curve is the parameter, so the curve is the control: nine handles on a
// square plot, dragged straight up and down, with the drive, mix, output and
// oversampling underneath as ordinary sliders.
//
// Like the Tone panel this is deliberately dumb — row DATA and a write block,
// never an engine pointer, because sinks die with their graph on every
// rebuild — and like it, the curve is not a second opinion about the DSP: it
// is plotted from engine::dsp::shaperCurveAt, the same spline the audio
// thread builds its table from.

#pragma once

#import <Cocoa/Cocoa.h>

#include <cstdint>

NS_ASSUME_NONNULL_BEGIN

@interface INCDAWShaperPanel : NSObject

/// Builds the window. `rows` are the shell's parameter rows for a
/// WaveshaperEffect slot; a row set that does not carry the shaper's ids is
/// rejected with nil, so the caller falls back to the generic panel.
/// `onWrite` receives PLAIN values.
+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    rows:(NSArray<NSDictionary*>*)rows
                                 onWrite:(void (^)(std::uint32_t parameterId,
                                                   double plainValue))onWrite;

/// Moves the handles and sliders to `values` (parameter id → plain value) —
/// how an open panel follows automation, MIDI knobs and undo. Ignored while
/// the mouse is down; a window that is not a shaper panel is ignored.
+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values;

/// Re-reads the palette into the panel's AppKit controls.
+ (void)refreshAppearance:(NSWindow*)window;

@end

NS_ASSUME_NONNULL_END
