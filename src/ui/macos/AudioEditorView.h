#pragma once

#import <Cocoa/Cocoa.h>

#include "engine/audio/WavFile.h"

#include <vector>

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

/// The markers stored in the open file, as the view last read them
/// (engine/audio/WavFile.h). Re-read by `reloadWaveform`.
@property (nonatomic, readonly) const std::vector<incdaw::engine::AudioMarker>& markers;

/// Frame under the last click, clamped to the file. Where a new marker goes
/// when there is no selection.
@property (nonatomic, readonly) long long caretFrame;

/// The marker nearest `frame` within `tolerance` frames, or -1. What "the
/// marker the user means" resolves to, without asking them to hit a pixel.
- (long long)markerIndexNear:(long long)frame within:(long long)tolerance;

@end
