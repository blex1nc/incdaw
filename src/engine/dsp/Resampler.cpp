#include "engine/dsp/Resampler.h"

#include <cmath>
#include <cstddef>

namespace incdaw::engine::dsp {

namespace {

constexpr double pi   = 3.14159265358979323846;
constexpr int    taps = 32;   ///< per side

double sinc(double x) noexcept
{
    if (std::abs(x) < 1.0e-9)
        return 1.0;

    const double px = pi * x;
    return std::sin(px) / px;
}

/// 4-term Blackman-Harris over [-1, 1]; ~92 dB sidelobe rejection.
double window(double position) noexcept
{
    if (position < -1.0 || position > 1.0)
        return 0.0;

    const double phase = pi * (position + 1.0);   // 0..2π across the window
    return 0.35875 - 0.48829 * std::cos(phase) + 0.14128 * std::cos(2.0 * phase)
         - 0.01168 * std::cos(3.0 * phase);
}

} // namespace

AudioFileData resample(const AudioFileData& source, SampleRate targetRate)
{
    AudioFileData result;
    result.sampleRate   = targetRate;
    result.channelCount = source.channelCount;

    if (source.sampleRate <= 0.0 || targetRate <= 0.0 || source.frameCount <= 0) {
        result.channels.assign(source.channelCount, {});
        return result;
    }

    const double ratio = source.sampleRate / targetRate;   // source frames per output frame

    // Rounded, not floored: 48000 frames at 48 kHz are exactly one second,
    // and one second at 44.1 kHz is exactly 44100 frames — floating-point
    // division must not shave a frame off that.
    const auto outputFrames = static_cast<FrameCount>(
        std::llround(static_cast<double>(source.frameCount) * targetRate / source.sampleRate));

    result.frameCount = outputFrames;
    result.channels.assign(source.channelCount,
                           std::vector<Sample>(static_cast<std::size_t>(outputFrames)));

    // Downsampling narrows the passband to the target Nyquist; upsampling
    // keeps the source's.
    const double cutoff = ratio > 1.0 ? 1.0 / ratio : 1.0;

    for (std::size_t channel = 0; channel < source.channelCount; ++channel) {
        const std::vector<Sample>& input  = source.channels[channel];
        std::vector<Sample>&       output = result.channels[channel];

        for (FrameCount frame = 0; frame < outputFrames; ++frame) {
            const double position = static_cast<double>(frame) * ratio;
            const auto   centre   = static_cast<FrameCount>(std::floor(position));
            const double fraction = position - static_cast<double>(centre);

            double sum = 0.0;

            for (int tap = -taps + 1; tap <= taps; ++tap) {
                const FrameCount index = centre + tap;
                if (index < 0 || index >= source.frameCount)
                    continue;   // zero-padded edges

                const double offset = static_cast<double>(tap) - fraction;
                const double weight = cutoff * sinc(cutoff * offset)
                                    * window(offset / static_cast<double>(taps));

                sum += static_cast<double>(input[static_cast<std::size_t>(index)]) * weight;
            }

            output[static_cast<std::size_t>(frame)] = static_cast<Sample>(sum);
        }
    }

    return result;
}

} // namespace incdaw::engine::dsp
