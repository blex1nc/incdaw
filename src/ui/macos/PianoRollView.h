#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::project { class Project; }
namespace incdaw::app     { class CommandRegistry; }

/// The Piano Roll editing surface.
///
/// Owns no model state of its own: geometry and hit testing come from
/// app::PianoRollModel, and every edit goes through app::CommandRegistry. The
/// view's entire job is turning input into commands and a draw list into
/// pixels (docs/ARCHITECTURE.md §2 and §6).
@interface INCDAWPianoRollView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                      project:(incdaw::project::Project*)project
                     registry:(incdaw::app::CommandRegistry*)registry;

/// Pattern being edited, by entity id value.
@property (nonatomic, assign) unsigned long long patternIdValue;

/// Channel being edited within that pattern. A pattern holds notes for every
/// channel programmed in it; the editor always looks at exactly one.
@property (nonatomic, assign) unsigned long long channelIdValue;

/// Playhead position in ticks; negative hides it.
@property (nonatomic, assign) long long playheadTick;

/// Text describing the last action, for the status line.
@property (nonatomic, copy, readonly) NSString* statusText;

/// Called after any edit, so the host can refresh its status line and rebuild
/// the playback graph.
@property (nonatomic, copy) void (^onChange)(void);

/// Called when the user asks to start or stop playback (space bar).
@property (nonatomic, copy) void (^onTransportToggle)(void);

/// Redraws on the next display-link tick. Used to animate the playhead without
/// the host having to know how frames are scheduled.
- (void)requestRedraw;

/// Drops selection indices that an undo or redo removed.
///
/// The selection is a list of positions in the pattern's note vector, so a
/// history step taken from anywhere — this view, another pane, the command
/// palette — can leave it pointing past the end, and the next draw would read
/// it. Whoever moves history calls this before that draw happens.
- (void)pruneSelectionAfterHistoryChange;

@end
