// The generic insert parameter panel: every parameter of one insert slot as
// a labelled slider. Builtin effects have no editor of their own, and some
// hosted plugins ship none — this panel is what "Open Editor" means for them.
//
// The panel is deliberately dumb: it holds row DATA and a write block, never
// an engine pointer. Sinks die with their graph on every rebuild, so the
// shell's block re-resolves the slot on each write instead of the panel
// caching anything it could dangle.

#pragma once

#import <Cocoa/Cocoa.h>

#include <cstdint>

NS_ASSUME_NONNULL_BEGIN

/// AppKit's default coordinate system grows upward; a list reads downward.
/// Shared by every hand-built row list in the shell (parameter panels, the
/// zone editor, the mapping list).
@interface INCDAWFlippedView : NSView
@end

/// Keys of one row dictionary: @"id" (unsigned), @"name" (NSString),
/// @"min", @"max", @"value" (double), @"stepped" (BOOL).
@interface INCDAWInsertParameterPanel : NSObject

/// Builds the window. `onWrite` is called with the parameter's PLAIN value on
/// every slider move; the caller owns routing it to the live sink.
+ (NSWindow*)makePanelWithTitle:(NSString*)title
                           rows:(NSArray<NSDictionary*>*)rows
                        onWrite:(void (^)(std::uint32_t parameterId, double plainValue))onWrite;

@end

NS_ASSUME_NONNULL_END
