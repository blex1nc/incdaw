#pragma once

#include "engine/audio/WavFile.h"

namespace incdaw::engine::edits {

/// Destructive editing primitives — the audio editor's verbs.
///
/// Pure functions over decoded audio: no I/O, no model, no undo. The command
/// layer above decides what to snapshot and when to write; keeping the DSP
/// here means the operations are testable sample-for-sample without a file
/// or a project anywhere in sight.
///
/// Regions are half-open [from, to) in frames and are clamped to the data,
/// so callers may pass a selection that outlives an edit that shrank the
/// audio and get the intersection rather than a crash.

struct Region {
    FrameCount from = 0;
    FrameCount to   = 0;

    [[nodiscard]] constexpr FrameCount length() const noexcept { return to - from; }
};

/// The region clamped to `data`, empty when there is no overlap.
[[nodiscard]] Region clampedRegion(const AudioFileData& data, Region region) noexcept;

/// Multiplies the region by `factor`.
void applyGain(AudioFileData& data, Region region, Sample factor) noexcept;

/// The largest absolute sample in the region, across channels.
[[nodiscard]] Sample peakIn(const AudioFileData& data, Region region) noexcept;

/// Scales the region so its peak lands on `targetPeak`. Returns false for a
/// silent region — scaling silence to a target is not a meaningful request,
/// and multiplying by infinity would be the alternative.
[[nodiscard]] bool normalize(AudioFileData& data, Region region, Sample targetPeak = 1.0f) noexcept;

/// Reverses the region in place, every channel. Its own inverse.
void reverse(AudioFileData& data, Region region) noexcept;

/// Zeroes the region.
void silence(AudioFileData& data, Region region) noexcept;

/// Linear ramp 0 -> 1 across the region.
void fadeIn(AudioFileData& data, Region region) noexcept;

/// Linear ramp 1 -> 0 across the region.
void fadeOut(AudioFileData& data, Region region) noexcept;

/// Keeps only the region: everything before and after is removed and the
/// audio shrinks to the region's length.
void trimTo(AudioFileData& data, Region region);

} // namespace incdaw::engine::edits
