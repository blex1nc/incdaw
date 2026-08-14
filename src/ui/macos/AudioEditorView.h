#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::project { class Project; }
namespace incdaw::app     { class CommandRegistry; }

/// The audio editor: one asset's waveform, a selection, and the destructive
/// verbs applied to it (via the Audio menu — the view itself only selects).
///
/// Owns no audio: the waveform is a WaveformOverview built through the
/// streaming reader, so opening an hour-long file costs its buckets, not its
/// samples. Edits rewrite the asset's file through the command registry, so
/// the editor shares the application's one undo stack; the host reloads the
/// waveform and rebuilds the playback graph after each edit.
///
/// CoreGraphics, like the playlist (D-015): a few thousand min/max columns.
@interface INCDAWAudioEditorView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                      project:(incdaw::project::Project*)project
                     registry:(incdaw::app::CommandRegistry*)registry;

/// The asset being edited; 0 shows the "nothing open" hint.
@property (nonatomic, assign) unsigned long long assetIdValue;

/// Rebuilds the overview from the asset's file. Called by the host after
/// every edit, undo or redo that may have touched the file.
- (void)reloadWaveform;

/// Selection in frames, half-open. `hasSelection` is false when empty.
@property (nonatomic, readonly) BOOL hasSelection;
@property (nonatomic, readonly) long long selectionFrom;
@property (nonatomic, readonly) long long selectionTo;

@end
