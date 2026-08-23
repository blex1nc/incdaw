#pragma once

#include "engine/audio/WavFile.h"
#include "engine/dsp/Stft.h"

#include <cstddef>
#include <vector>

namespace incdaw::engine::dsp {

/// What the room sounds like when nobody is playing.
///
/// A mean magnitude per frequency bin, learned from a stretch of the recording
/// the user says is silence. Per channel rather than shared: a two-microphone
/// take has two noise floors, and averaging them subtracts each one's hum from
/// the other's.
///
/// The analysis parameters are carried WITH the spectrum. A profile learned at
/// one FFT size means nothing at another — the bins are different frequencies —
/// and a profile that did not say so would be silently misapplied.
struct NoiseProfile {
    std::size_t fftSize    = 0;
    SampleRate  sampleRate = 0.0;

    /// One vector of `fftSize / 2 + 1` magnitudes per channel.
    std::vector<std::vector<float>> channels;

    [[nodiscard]] bool isEmpty() const noexcept { return channels.empty(); }
};

/// Learns a profile from `region`, which the user has told us is silence.
///
/// Returns an empty profile for a region too short to analyse: one window is
/// the minimum, and a profile learned from less would describe the window
/// rather than the room.
[[nodiscard]] NoiseProfile learnNoiseProfile(const AudioFileData& data,
                                             FramePosition        from,
                                             FramePosition        to,
                                             std::size_t          fftSize = Stft::defaultFftSize);

/// Subtracts `profile` across `region`.
///
/// `amount` scales the profile: 1 subtracts what was measured, more is more
/// aggressive, 0 does nothing at all — and doing nothing at all leaves the
/// audio bit-identical rather than passing it through a round trip, because a
/// user who cancels a denoise should get their file back, not a re-rendered
/// copy of it.
///
/// `floorGain` is the least a bin may be reduced to. Subtracting all the way
/// to zero is what produces "musical noise" — isolated surviving bins warbling
/// against silence — and it sounds far worse than the hiss it removed.
///
/// False when the profile does not match the audio (rate or FFT size) or the
/// region is empty. Offline: allocates freely, never on the audio thread.
[[nodiscard]] bool denoise(AudioFileData& data, FramePosition from, FramePosition to,
                           const NoiseProfile& profile, double amount = 1.0,
                           double floorGain = 0.05);

} // namespace incdaw::engine::dsp
