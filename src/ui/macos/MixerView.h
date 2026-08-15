#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::project { class Project; }
namespace incdaw::app     { class CommandRegistry; }
namespace incdaw::engine::dsp { class MixerStripNode; }

/// The mixer: one strip per mixer node, with a fader, pan, mute, solo, polarity
/// and a meter.
///
/// Meters are read from the live graph rather than from the model, because a
/// level is not project state — it is what the audio thread just produced. The
/// host supplies a lookup from mixer node id to the strip currently rendering
/// it; the view never holds those pointers across a rebuild.
@interface INCDAWMixerView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                      project:(incdaw::project::Project*)project
                     registry:(incdaw::app::CommandRegistry*)registry;

/// Channel whose routing the mixer acts on ("route this channel here").
@property (nonatomic, assign) unsigned long long selectedChannelIdValue;

/// Returns the strip rendering a mixer node right now, or nullptr.
@property (nonatomic, copy) incdaw::engine::dsp::MixerStripNode* (^stripLookup)(unsigned long long);

/// Called after an edit that changes the graph's shape — a track added, routing
/// changed — so the host recompiles.
@property (nonatomic, copy) void (^onChange)(void);

/// Called after an edit that only changes a parameter. The host does NOT
/// recompile for these: the value has already been written to the live strip,
/// and rebuilding the graph would reset every meter to draw the same audio.
@property (nonatomic, copy) void (^onParameterChange)(void);

/// A specific fader or pan move, in the parameter registry's normalised
/// terms — what automation write mode records. `key` is "volume" or "pan".
@property (nonatomic, copy) void (^onParameterEdited)(unsigned long long nodeId,
                                                      const char* key, double normalized);

/// Called when the user asks to start or stop playback (space bar).
@property (nonatomic, copy) void (^onTransportToggle)(void);

/// The plugins the user can add as inserts, from the registry the shell
/// owns. Each entry: @{@"uid": ..., @"name": ...}. The view knows menus,
/// not plugin catalogues.
@property (nonatomic, copy) NSArray<NSDictionary*>* availableInserts;

/// Asks the shell to open the slot's plugin editor window. The shell owns
/// windows and instances; the view only knows the slot was asked for.
@property (nonatomic, copy) void (^onOpenInsertEditor)(unsigned long long slotId);

/// Arms MIDI learn for a parameter of a mixer node: the next CC that arrives
/// becomes the mapping. The shell owns the armed state and the poll.
@property (nonatomic, copy) void (^onMidiLearn)(NSString* parameterKey,
                                                unsigned long long targetId);

/// Removes every MIDI mapping targeting the node.
@property (nonatomic, copy) void (^onMidiForget)(unsigned long long targetId);

@end
