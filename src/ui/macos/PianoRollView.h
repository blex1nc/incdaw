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

/// Playhead position in ticks; negative hides it.
@property (nonatomic, assign) long long playheadTick;

/// Text describing the last action, for the status line.
@property (nonatomic, copy, readonly) NSString* statusText;

/// Called after any edit, so the host window can refresh its status line.
@property (nonatomic, copy) void (^onChange)(void);

@end
