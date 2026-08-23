#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::project { class Project; class ParameterRegistry; }
namespace incdaw::app     { class CommandRegistry; }

/// The playlist: tracks down the side, pattern clips on a timeline.
///
/// Owns no model state. Geometry and hit testing come from app::PlaylistModel
/// and every edit goes through app::CommandRegistry, so playlist edits share
/// the one undo stack with the Piano Roll and the Channel Rack.
///
/// CoreGraphics rather than Metal, like the Channel Rack (D-015): the viewport
/// holds tens of clips, and culling in PlaylistModel keeps it that way however
/// long the song gets.
@interface INCDAWPlaylistView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                      project:(incdaw::project::Project*)project
                     registry:(incdaw::app::CommandRegistry*)registry;

/// Pattern painted into empty timeline space, and shown in the toolbar.
@property (nonatomic, assign) unsigned long long patternIdValue;

/// Playhead position in ticks; negative hides it.
@property (nonatomic, assign) long long playheadTick;

/// Called after any edit, so the host can rebuild the playback graph.
@property (nonatomic, copy) void (^onChange)(void);

/// The host's parameter registry, used when a consolidation renders. Optional:
/// without it a render still applies builtin and stored instrument parameters,
/// which is what the compiler does on its own.
@property (nonatomic, assign) const incdaw::project::ParameterRegistry* parameterRegistry;

/// Called when the user clicks the ruler, to move the transport.
@property (nonatomic, copy) void (^onSeekTick)(long long);

/// Called when the user asks to start or stop playback (space bar).
@property (nonatomic, copy) void (^onTransportToggle)(void);

/// Called when the user double-clicks an audio clip, with the clip's asset
/// id — the host opens it in the audio editor.
@property (nonatomic, copy) void (^onOpenAudioAsset)(unsigned long long);

/// Whether the clips before the start marker are triggered rather than played
/// in sequence. The host reads this when it compiles, so changing it asks for
/// a rebuild through `onPerformanceModeChanged`.
@property (nonatomic, assign) BOOL performanceMode;

/// Called when the mode is switched, so the host can recompile the graph.
@property (nonatomic, copy) void (^onPerformanceModeChanged)(void);

/// Called when a pad is pressed or released: the track it belongs to, the pad,
/// and whether it went down. The host resolves it against the compiled graph,
/// which is the only place that knows which scheduler slot a track is.
@property (nonatomic, copy) void (^onPerformanceTrigger)(unsigned long long, int, bool);

/// Drops cached clip waveforms. The host calls this after any edit, undo or
/// redo that may have rewritten an asset's file.
- (void)invalidateWaveformCache;

@end
