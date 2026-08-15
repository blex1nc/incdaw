#pragma once

#include "engine/audio/WavFile.h"

namespace incdaw::engine::dsp {

/// Offline sample-rate conversion: windowed-sinc interpolation
/// (Blackman-Harris window, 32 taps per side), lowpassed to the narrower
/// Nyquist when downsampling.
///
/// Offline only, by design: it allocates the result and takes its time.
/// The REALTIME repitching in the sampler is a different tool for a
/// different job — this one is for export quality, where "linear
/// interpolation" would not be an honest answer (CLAUDE.md §16's rule
/// against low-quality placeholders applies to export too).
[[nodiscard]] AudioFileData resample(const AudioFileData& source, SampleRate targetRate);

} // namespace incdaw::engine::dsp
