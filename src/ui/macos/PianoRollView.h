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

/// Grid resolution in ticks; zero snaps to nothing. The same value the editor's
/// own quantize, note-length and drag arithmetic read, so a control strip that
/// sets it cannot disagree with the grid the notes land on.
@property (nonatomic, assign) long long snapTicks;

/// The key signature the scale highlighting and the note-nudge tool both work
/// in: root pitch class (0 = C) and an app::music::Scale as its index.
@property (nonatomic, assign) int keyRootPitchClass;
@property (nonatomic, assign) int scaleIndex;

@property (nonatomic, assign) BOOL ghostNotesVisible;
@property (nonatomic, assign) BOOL velocityLaneVisible;

/// Called when one of the settings above is changed from inside the editor —
/// by a keystroke, not by a setter — so a control strip showing them follows.
@property (nonatomic, copy) void (^onEditorStateChanged)(void);

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
