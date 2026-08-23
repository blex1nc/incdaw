#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::project { class Project; }
namespace incdaw::app     { class CommandRegistry; }

/// The automation editor: one lane's envelope, drawn and edited.
///
/// Owns no model state beyond the view's own — geometry, hit testing and every
/// edit come from app::AutomationEditorModel, and each edit goes through
/// app::SetAutomationPointsCommand, so an automation edit shares the one undo
/// stack with the playlist, the Piano Roll and the mixer.
///
/// The curve it draws is evaluated by the class the audio thread reads
/// (engine::AutomationSequence, through the model), so what is on screen is
/// what is being played rather than a picture of it.
@interface INCDAWAutomationEditorView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                      project:(incdaw::project::Project*)project
                     registry:(incdaw::app::CommandRegistry*)registry;

/// The lane being edited. Setting it clears the selection and redraws.
@property (nonatomic, assign) unsigned long long laneIdValue;

/// Playhead position in ticks; negative hides it.
@property (nonatomic, assign) long long playheadTick;

/// Called after any edit, so the host can rebuild the playback graph.
@property (nonatomic, copy) void (^onChange)(void);

/// Text describing the lane and the last action, for the panel's title.
@property (nonatomic, copy, readonly) NSString* statusText;

@end
