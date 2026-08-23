#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::project { class Project; }
namespace incdaw::app     { class CommandRegistry; }

/// The comping editor: one recorded take per lane, and a range assigned to a
/// take by dragging across it.
///
/// Loop recording has stacked passes since Phase 12 — one file, one clip per
/// pass, every pass but the last muted — and nothing chose between them. This
/// is the choosing. Each drag is one `AssignCompRangeCommand`, so the composite
/// is audible the moment the drag ends and Cmd+Z peels exactly the last
/// decision rather than the whole comp.
///
/// Deliberately its own window rather than lanes inside the Playlist. A comp is
/// made by listening to one span over and over, which wants the takes big and
/// side by side; the arrangement wants them collapsed to the one line the comp
/// produces. Those are different views of the same clips, and the model already
/// holds both because a comp IS the clips.
@interface INCDAWCompingView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                      project:(incdaw::project::Project*)project
                     registry:(incdaw::app::CommandRegistry*)registry;

/// The track whose takes are shown, and the span being comped. Setting either
/// re-reads the stack.
@property (nonatomic, assign) unsigned long long trackIdValue;
@property (nonatomic, assign) long long spanFrom;
@property (nonatomic, assign) long long spanTo;

/// Re-reads the takes and the waveform. Called after every assignment, undo
/// or redo.
- (void)reload;

/// Called after an assignment lands, on the main thread: the host rebuilds the
/// graph so the composite is what plays.
@property (nonatomic, copy) void (^onCompChanged)(void);

/// How many lanes the stack has. Zero means "nothing to comp here", which the
/// view says in words rather than by drawing an empty grid.
@property (nonatomic, readonly) NSInteger laneCount;

@end
