#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::app { class Browser; }

/// The Browser pane: the user's disk, as INCDAW sees it.
///
/// Unlike the musical surfaces in this shell, this one is a stock
/// NSOutlineView rather than a custom-drawn view. A file tree is exactly what
/// AppKit's outline view is for, and hand-drawing one would buy nothing but
/// bugs. Everything it shows comes from app::Browser, so the ordering,
/// classification and search behaviour that the tests assert are literally
/// what appears here.
///
/// The pane opens nothing itself. Double-clicking hands the path back to the
/// shell, which owns what "open" means for a project, a MIDI file or a sample.
@interface INCDAWBrowserView : NSView

- (instancetype)initWithFrame:(NSRect)frame browser:(incdaw::app::Browser*)browser;

/// Double-click on something that is not a folder.
@property (nonatomic, copy) void (^onActivate)(NSString* path);

/// Roots, favourites or recents changed and want writing back to disk.
@property (nonatomic, copy) void (^onSettingsChanged)(void);

/// Re-reads the tree from disk, forgetting every cached folder listing.
- (void)reload;

@end
