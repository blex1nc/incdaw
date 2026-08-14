#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::project { class Project; }
namespace incdaw::app     { class CommandRegistry; }

/// The Channel Rack: one row per channel, with its step grid.
///
/// Owns no model state. Geometry and hit testing come from
/// app::ChannelRackModel, and every edit goes through app::CommandRegistry, so
/// a rack edit and a Piano Roll edit share one undo stack
/// (docs/ARCHITECTURE.md §6).
///
/// Drawn with CoreGraphics rather than Metal, unlike the Piano Roll: a rack is
/// tens of rectangles against ten thousand notes, and the GPU path costs a
/// shader, a pipeline and a display link to save nothing measurable (D-015).
@interface INCDAWChannelRackView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                      project:(incdaw::project::Project*)project
                     registry:(incdaw::app::CommandRegistry*)registry;

/// Pattern whose steps are shown.
@property (nonatomic, assign) unsigned long long patternIdValue;

/// Channel the Piano Roll is editing, highlighted here.
@property (nonatomic, assign) unsigned long long selectedChannelIdValue;

/// Playhead position in ticks; negative hides the step highlight.
@property (nonatomic, assign) long long playheadTick;

/// Called after any edit, so the host can rebuild the playback graph.
@property (nonatomic, copy) void (^onChange)(void);

/// Called when the user picks a channel to edit.
@property (nonatomic, copy) void (^onSelectChannel)(unsigned long long);

/// Called when the user asks to start or stop playback (space bar).
@property (nonatomic, copy) void (^onTransportToggle)(void);

@end
