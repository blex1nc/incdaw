#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::app { class BrowserModel; }

/// The browser pane (CLAUDE.md §19).
///
/// A DAW without one is a DAW where every sample arrives through an open
/// panel: no search, no favourites, no dragging a kick onto a channel. The
/// pane navigates the folders the user has added as roots, searches them, and
/// hands files to the shell — it loads nothing itself, because what "open this
/// file" means (a sample onto a channel, a project, a MIDI import) is the
/// application's decision, not the browser's.
///
/// The model behind it is `app::BrowserModel`, which is headless and tested;
/// this view only draws it and turns gestures into callbacks.
@interface INCDAWBrowserView : NSView

- (instancetype)initWithFrame:(NSRect)frame browser:(incdaw::app::BrowserModel*)browser;

/// A file was opened (double-click, or Return). The shell decides what that
/// means for the file's kind.
@property (nonatomic, copy) void (^onActivateFile)(NSString* path);

/// Roots or favourites changed and should be persisted.
@property (nonatomic, copy) void (^onLibraryChanged)(void);

/// Space bar, like every other pane: the transport belongs to the application.
@property (nonatomic, copy) void (^onTransportToggle)(void);

/// Re-reads the current folder from disk. Cheap, and what a Rescan or an
/// external change calls.
- (void)reload;

@end
