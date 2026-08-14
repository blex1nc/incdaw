#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::project { class Project; }
namespace incdaw::app     { class CommandRegistry; }

/// The Playlist: the arrangement's editing surface.
///
/// Same contract as the Piano Roll — no model state of its own, geometry from
/// app::PlaylistModel, every edit through app::CommandRegistry — and the same
/// renderer, because both surfaces are rectangles.
@interface INCDAWPlaylistView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                      project:(incdaw::project::Project*)project
                     registry:(incdaw::app::CommandRegistry*)registry;

/// Pattern placed when the user clicks empty timeline, by entity id value.
@property (nonatomic, assign) unsigned long long patternIdValue;

/// Playhead position in frames; negative hides it.
@property (nonatomic, assign) long long playheadFrame;

/// Called after any edit, so the host can rebuild the graph.
@property (nonatomic, copy) void (^onChange)(void);

/// Called when the user asks to start or stop playback (space bar).
@property (nonatomic, copy) void (^onTransportToggle)(void);

- (void)requestRedraw;

@end
