#pragma once

#include "engine/audio/WavFile.h"

namespace incdaw::engine::dsp {

/// Offline time and pitch processing (docs/FL2026_GAP.md P7; CLAUDE.md §16).
///
/// The algorithm is WSOLA — waveform-similarity overlap-add — with transient
/// locking: segment alignment is chosen by cross-correlation on a mono mix
/// (one offset for every channel, so stereo phase survives), and detected
/// onsets are consumed exactly once, so a stretched drum hit neither doubles
/// nor vanishes. Pitch shifting is a stretch followed by windowed-sinc
/// resampling, which trades no quality for simplicity offline.
///
/// Offline by design, like `resample`: it allocates and takes its time.
/// A realtime preview path can join later behind this same options struct —
/// that is the "pluggable" in the requirement.
struct StretchOptions {
    /// Output duration as a multiple of the input's: 2.0 doubles it.
    double ratio = 1.0;

    /// Pitch shift in semitones, independent of `ratio`.
    double pitchSemitones = 0.0;
};

/// A ratio-1, zero-semitone call returns the input untouched — the identity
/// is exact, never "almost".
[[nodiscard]] AudioFileData timeStretch(const AudioFileData& source,
                                        const StretchOptions& options);

} // namespace incdaw::engine::dsp
