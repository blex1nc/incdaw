#pragma once

#import <Cocoa/Cocoa.h>

#include <cstddef>

namespace incdaw::project { class Project; }
namespace incdaw::app     { class CommandRegistry; }

/// The Channel Rack: the project's channels, and one pattern as a step grid.
///
/// Owns no model state. A step is a note (app::StepSequencerModel), a click is
/// a command (app::ToggleStepCommand), and the view's whole job is turning
/// input into commands and the model into pixels — the same contract the Piano
/// Roll works under (docs/ARCHITECTURE.md §2 and §6).
///
/// Drawn with Core Graphics rather than Metal: a rack is tens of rectangles
/// that change when the user clicks, not thousands that change every frame.
@interface INCDAWChannelRackView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                      project:(incdaw::project::Project*)project
                     registry:(incdaw::app::CommandRegistry*)registry;

/// Pattern whose steps are shown, by entity id value.
@property (nonatomic, assign) unsigned long long patternIdValue;

/// Channel the Piano Roll is editing, by entity id value.
@property (nonatomic, assign) unsigned long long selectedChannelIdValue;

/// Playhead position in ticks; negative hides the step cursor.
@property (nonatomic, assign) long long playheadTick;

/// Called after any edit that changes what plays, so the host can rebuild the
/// graph and refresh its status line.
@property (nonatomic, copy) void (^onChange)(void);

/// Called when the user selects a different channel.
@property (nonatomic, copy) void (^onChannelSelected)(void);

/// Height the rack needs for a given number of channels, so the window can lay
/// it out without duplicating the row arithmetic.
+ (double)heightForChannelCount:(std::size_t)count;

@end
