// The device panel: one view that renders any DeviceUiSpec.
//
// A spec is a tree of widgets naming parameter ids (app/devices/
// DeviceUiSpec.h); this view draws it from the theme's primitives, writes
// plain values back through one block, and follows the live values the shell
// hands it. Same discipline as the generic parameter panel: the view holds
// row DATA and a write block, never an engine pointer — sinks die with their
// graph on every rebuild, and the shell re-resolves the slot on each write.
//
// Once "Open Editor" dispatches through this, no device needs a line in the
// shell: a new panel is a spec in app/devices/, and a spec the renderer does
// not yet fully draw still opens (the widgets it cannot draw are labelled
// wells), so specs may ship ahead of the renderer.

#pragma once

#import <Cocoa/Cocoa.h>

#include "app/devices/DeviceUiSpec.h"

#include <cstdint>

NS_ASSUME_NONNULL_BEGIN

/// The escape hatch: a bespoke view for a surface the vocabulary cannot
/// carry (`DeviceUiSpec::customView` names the class). Budget: eight
/// devices across the archive, each justified in docs/DECISIONS.md.
@protocol INCDAWDeviceCustomView <NSObject>

/// Builds the view. `rows` are the shell's parameter rows (keys @"id",
/// @"name", @"min", @"max", @"default", @"value", @"stepped"); `onWrite`
/// receives PLAIN values. Return nil to fall back to the generic panel.
+ (nullable NSView*)makeViewWithRows:(NSArray<NSDictionary*>*)rows
                          sampleRate:(double)sampleRate
                             onWrite:(void (^)(std::uint32_t parameterId,
                                               double plainValue))onWrite;

/// Live values (parameter id → plain value), ~5 Hz. Ignore during a drag.
- (void)refreshValues:(NSDictionary<NSNumber*, NSNumber*>*)values;

@end

@interface INCDAWDevicePanel : NSObject

/// Builds the window for `spec`. Returns nil — and the caller falls back to
/// the generic panel — when a widget names a parameter `rows` does not
/// carry, or when `spec->customView` names a class that is not there.
/// `spec` must be static storage (a catalogue entry). `sampleRate` is the
/// device's, used only to plot response curves.
+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    spec:(const incdaw::app::DeviceUiSpec*)spec
                                    rows:(NSArray<NSDictionary*>*)rows
                              sampleRate:(double)sampleRate
                                 onWrite:(void (^)(std::uint32_t parameterId,
                                                   double plainValue))onWrite;

/// Moves the controls to `values` — how an open panel follows automation,
/// MIDI knobs and undo. Ignored while the mouse is down; a window that is
/// not a device panel is ignored entirely.
+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values;

/// Re-reads the palette. The panel draws itself, so this only re-grounds the
/// window and invalidates; kept for symmetry with the generic panel.
+ (void)refreshAppearance:(NSWindow*)window;

/// True when `window` was built by this class.
+ (BOOL)isDevicePanel:(NSWindow*)window;

@end

NS_ASSUME_NONNULL_END
