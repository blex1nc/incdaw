#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::project { class Project; }
namespace incdaw::app     { class CommandRegistry; }

/// The pattern list.
///
/// Selecting a pattern retargets both editors and what the transport loops; it
/// is not a copy of anything. A pattern placed several times in an arrangement
/// (Phase 9) will still be this one pattern, which is why editing it changes
/// every placement.
@interface INCDAWPatternListView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                      project:(incdaw::project::Project*)project
                     registry:(incdaw::app::CommandRegistry*)registry;

@property (nonatomic, assign) unsigned long long selectedPatternIdValue;

/// Called after any edit, so the host can rebuild the playback graph.
@property (nonatomic, copy) void (^onChange)(void);

/// Called when the user picks a pattern.
@property (nonatomic, copy) void (^onSelectPattern)(unsigned long long);

/// Called when the user asks to start or stop playback (space bar).
///
/// Every pane handles space, because the transport belongs to the application
/// rather than to whichever editor happens to hold focus.
@property (nonatomic, copy) void (^onTransportToggle)(void);

@end
