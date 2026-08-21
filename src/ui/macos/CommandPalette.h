#pragma once

#import <Cocoa/Cocoa.h>

/// One thing the application can do, as the palette sees it.
///
/// Deliberately not a command: the palette lists menu entries, registered
/// project commands and shell actions side by side, and what they have in
/// common is a name and something to run — not a class.
@interface INCDAWCommandEntry : NSObject

@property (nonatomic, copy) NSString* title;
@property (nonatomic, copy) NSString* category;
@property (nonatomic, copy) NSString* shortcut;      ///< "⌘S", or empty
@property (nonatomic, copy) void (^run)(void);

+ (instancetype)entryWithTitle:(NSString*)title
                      category:(NSString*)category
                      shortcut:(NSString*)shortcut
                           run:(void (^)(void))run;

@end

/// Command search (CLAUDE.md §26).
///
/// A DAW accumulates hundreds of actions, and a menu bar stops being a way to
/// find them long before that. The palette is the answer every large
/// application has converged on: one key, type a few letters, run it — and it
/// is only possible because actions are addressable rather than buried in
/// whichever view happens to implement them.
///
/// The palette owns no catalogue. The shell hands it entries each time it
/// opens, gathered from the menu bar and `app::CommandRegistry`, so a command
/// can never be listed here and missing there.
@interface INCDAWCommandPalette : NSObject

- (void)showWithEntries:(NSArray<INCDAWCommandEntry*>*)entries
       relativeToWindow:(NSWindow*)window;

- (void)close;

@end
