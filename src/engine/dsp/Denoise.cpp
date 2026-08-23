#include "engine/dsp/Denoise.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine::dsp {
namespace {

/// [from, to) clamped to the audio, as indices.
std::pair<std::size_t, std::size_t> clampSpan(const AudioFileData& data, FramePosition from,
                                              FramePosition to) noexcept
{
    const auto limit = static_cast<FramePosition>(data.frameCount);

    const FramePosition low  = std::clamp<FramePosition>(from, 0, limit);
    const FramePosition high = std::clamp<FramePosition>(to, low, limit);

    return {static_cast<std::size_t>(low), static_cast<std::size_t>(high)};
}

} // namespace

NoiseProfile learnNoiseProfile(const AudioFileData& data, FramePosition from, FramePosition to,
                               std::size_t fftSize)
{
    NoiseProfile profile;

    const auto [low, high] = clampSpan(data, from, to);
    if (high <= low || data.channels.empty())
        return profile;

    const Stft stft{fftSize};

    // One window is the minimum. Less than that describes the window rather
    // than the room, and a profile built from a fragment removes the wrong
    // thing everywhere it is applied.
    if (high - low < stft.fftSize())
        return profile;

    profile.fftSize    = stft.fftSize();
    profile.sampleRate = data.sampleRate;

    for (const auto& channel : data.channels) {
        std::vector<Sample> slice(channel.begin() + static_cast<std::ptrdiff_t>(low),
                                  channel.begin() + static_cast<std::ptrdiff_t>(
                                      std::min(high, channel.size())));

        std::vector<double> sum(stft.binCount(), 0.0);
        std::size_t         frames = 0;

        stft.analyse(slice, [&](std::size_t, const std::vector<float>& magnitudes) {
            for (std::size_t bin = 0; bin < sum.size() && bin < magnitudes.size(); ++bin)
                sum[bin] += static_cast<double>(magnitudes[bin]);

            ++frames;
        });

        std::vector<float> mean(stft.binCount(), 0.0f);

        // The mean, not the peak. A peak profile subtracts the loudest moment
        // of the noise everywhere, which takes the quiet parts of the signal
        // with it.
        if (frames > 0)
            for (std::size_t bin = 0; bin < mean.size(); ++bin)
                mean[bin] = static_cast<float>(sum[bin] / static_cast<double>(frames));

        profile.channels.push_back(std::move(mean));
    }

    return profile;
}

bool denoise(AudioFileData& data, FramePosition from, FramePosition to,
             const NoiseProfile& profile, double amount, double floorGain)
{
    if (profile.isEmpty() || profile.fftSize == 0)
        return false;

    // A profile learned at another rate describes other frequencies. Applying
    // it would remove a hum that is not there and leave the one that is.
    if (profile.sampleRate > 0.0 && data.sampleRate > 0.0
        && std::abs(profile.sampleRate - data.sampleRate) > 1.0)
        return false;

    const auto [low, high] = clampSpan(data, from, to);
    if (high <= low)
        return false;

    // Nothing to do, and deliberately without a round trip: a user who dials
    // the amount to zero should get their file back, not a re-rendered copy
    // of it that differs in the last bit.
    if (amount <= 0.0)
        return true;

    const Stft stft{profile.fftSize};
    const auto clampedFloor = static_cast<float>(std::clamp(floorGain, 0.0, 1.0));

    for (std::size_t channel = 0; channel < data.channels.size(); ++channel) {
        // A profile with fewer channels than the audio reuses its last one —
        // which is what a mono profile applied to a stereo take should do.
        const std::vector<float>& spectrum =
            profile.channels[std::min(channel, profile.channels.size() - 1)];

        if (spectrum.size() != stft.binCount())
            return false;

        auto& samples = data.channels[channel];
        if (samples.size() < high)
            continue;

        std::vector<Sample> slice(samples.begin() + static_cast<std::ptrdiff_t>(low),
                                  samples.begin() + static_cast<std::ptrdiff_t>(high));

        const auto processed = stft.process(slice, [&](std::size_t, float* real, float* imaginary) {
            const std::size_t bins = stft.binCount();

            for (std::size_t bin = 0; bin < bins; ++bin) {
                const auto magnitude = std::hypot(static_cast<double>(real[bin]),
                                                  static_cast<double>(imaginary[bin]));

                if (magnitude <= 1.0e-12)
                    continue;

                const double subtracted = amount * static_cast<double>(spectrum[bin]);
                const auto   gain       = static_cast<float>(
                    std::max(static_cast<double>(clampedFloor), 1.0 - subtracted / magnitude));

                real[bin]      *= gain;
                imaginary[bin] *= gain;

                // The upper half is the conjugate mirror of the lower. Scaling
                // only the half-spectrum leaves an asymmetric transform, whose
                // inverse is complex — and the imaginary part is discarded, so
                // the result is quietly half the amplitude it should be.
                if (bin > 0 && bin < bins - 1) {
                    const std::size_t mirror = stft.fftSize() - bin;
                    real[mirror]      *= gain;
                    imaginary[mirror] *= gain;
                }
            }
        });

        std::copy(processed.begin(), processed.end(),
                  samples.begin() + static_cast<std::ptrdiff_t>(low));
    }

    return true;
}

} // namespace incdaw::engine::dsp
