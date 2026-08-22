// The preset control that sits above a parameter panel (A5).
//
// One strip: the name of what is loaded, and everything one can do to it.
// Factored out of the panels rather than written into each because the
// generic insert panel, the Tone panel and the instrument panel all need the
// identical control, and three copies of a menu that can save files is three
// places for "Delete" to mean something slightly different.
//
// The bar knows nothing about presets as data. It is handed a list of names,
// and it reports intentions — recall this, save under that name, rename this
// to that. Where the files live and what a recall does to the engine belong
// to the shell.

#pragma once

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

/// Height the bar wants. Panels reserve this above their own content.
extern const CGFloat INCDAWPresetBarHeight;

@interface INCDAWPresetBar : NSView

/// Inserts a bar at the top of `window`, growing the window by the bar's
/// height and keeping whatever was already there below it.
///
/// Attaching from the outside rather than building the bar into each panel is
/// what lets the generic parameter panel, the Tone panel and anything added
/// later share one control without any of them knowing it exists. Calling
/// twice returns the bar already attached.
+ (INCDAWPresetBar*)attachToWindow:(NSWindow*)window;

/// The bar attached to `window`, or nil. A window that is not a parameter
/// panel is simply one without a bar.
+ (nullable INCDAWPresetBar*)barInWindow:(NSWindow*)window;

/// Re-reads the palette into `window`'s bar, if it has one.
+ (void)refreshAppearanceInWindow:(NSWindow*)window;

/// `entries` are dictionaries of @"name" (NSString) and @"factory" (BOOL).
/// `selected` may be nil, which shows the bar's "unsaved" state — what a
/// panel looks like after a slider has moved away from the preset it loaded.
- (void)setEntries:(NSArray<NSDictionary*>*)entries selected:(nullable NSString*)selected;

/// The name currently shown, or nil when nothing is loaded.
@property (nonatomic, copy, readonly, nullable) NSString* selectedName;

/// Called when the user picks a preset from the list.
@property (nonatomic, copy, nullable) void (^onRecall)(NSString* name);

/// Called with a name the user typed, having already confirmed any overwrite.
@property (nonatomic, copy, nullable) void (^onSave)(NSString* name);

@property (nonatomic, copy, nullable) void (^onRename)(NSString* from, NSString* to);
@property (nonatomic, copy, nullable) void (^onDuplicate)(NSString* name);
@property (nonatomic, copy, nullable) void (^onDelete)(NSString* name);

/// Re-reads the palette into the controls. A label's colour is a snapshot
/// handed over once, so a live theme change reaches it only by reassignment.
- (void)refreshAppearance;

@end

NS_ASSUME_NONNULL_END
