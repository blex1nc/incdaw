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

// ── Markers ──────────────────────────────────────────────────────────────────
//
// The length-changing verbs below keep `data.markers` coherent, because the
// alternative is markers that quietly stop meaning anything: delete a bar from
// the middle of a file and every marker after it is now pointing at the wrong
// sound, with nothing to say so. The rules are the obvious ones written down —
//
//   * audio inserted before a marker pushes it later;
//   * audio removed under a point marker removes the marker with it;
//   * a region marker that straddles a deletion keeps the part that survived;
//   * a trim rebases what it kept and drops what it did not.
//
// The verbs that do not change length — gain, fades, reverse, silence — leave
// markers exactly where they are. Reversing a region does NOT mirror the
// markers inside it: a marker names a moment in the material, and the material
// is what was reversed.

/// Shifts markers at or after `at` by `delta`, dropping any that would land
/// before zero. Exposed because the commands need the same arithmetic when
/// they replay an edit.
void shiftMarkers(AudioFileData& data, FramePosition at, FrameCount delta);

/// Removes `region` from the marker list, closing the gap the way
/// `deleteRegion` closes it in the audio.
void removeMarkersIn(AudioFileData& data, Region region);

/// Keeps only the region: everything before and after is removed and the
/// audio shrinks to the region's length.
void trimTo(AudioFileData& data, Region region);

/// The region's samples as their own audio — the clipboard's copy verb.
/// Metadata (rate, channel count) rides along; markers deliberately do not.
/// Pasting a copied span should add sound, not someone else's annotations.
[[nodiscard]] AudioFileData extractRegion(const AudioFileData& data, Region region);

/// Removes the region and closes the gap; the audio shrinks by its length.
void deleteRegion(AudioFileData& data, Region region);

/// Inserts `piece` at frame `at` (clamped to the end), pushing what follows
/// later. False — and untouched data — when the piece's sample rate or
/// channel count disagrees: a paste must never resample or remap silently.
[[nodiscard]] bool insertAudio(AudioFileData& data, FramePosition at,
                               const AudioFileData& piece);

} // namespace incdaw::engine::edits
