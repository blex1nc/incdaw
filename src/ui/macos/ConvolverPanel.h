// The Convolution Reverb's editor: the ordinary sliders, plus the one thing
// no slider can carry — which impulse response is loaded.
//
// INCDAW ships no impulses (§20/§43), so the file field is not a convenience:
// it is how the effect becomes a particular room at all. A convolver with no
// file plays a hall generated from code, and the field says so.

#pragma once

#import <Cocoa/Cocoa.h>

#include <cstdint>

NS_ASSUME_NONNULL_BEGIN

@interface INCDAWConvolverPanel : NSObject

/// Builds the window. `rows` are the shell's parameter rows for a
/// ConvolutionReverbEffect slot; a row set that does not carry its ids is
/// rejected with nil. `impulse` is the path currently loaded, or nil for the
/// generated one. `onWrite` receives PLAIN parameter values; `onImpulse`
/// receives a chosen path, or nil to go back to the generated hall.
+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    rows:(NSArray<NSDictionary*>*)rows
                                 impulse:(nullable NSString*)impulse
                                 onWrite:(void (^)(std::uint32_t parameterId,
                                                   double plainValue))onWrite
                               onImpulse:(void (^)(NSString* _Nullable path))onImpulse;

/// Moves the sliders to `values`, and the file field to `impulse`.
+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values
              impulse:(nullable NSString*)impulse;

/// Re-reads the palette into the panel's AppKit controls.
+ (void)refreshAppearance:(NSWindow*)window;

@end

NS_ASSUME_NONNULL_END
