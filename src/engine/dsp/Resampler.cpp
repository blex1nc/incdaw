#include "engine/dsp/Resampler.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

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

/// The windowed-sinc kernel, precomputed at `phases` fractional positions.
///
/// Evaluating sinc and the window per tap per output frame cost two
/// transcendental calls per tap — 18x realtime for a 10 s stereo file
/// (docs/PERFORMANCE.md, Phase 18). The table trades that for one build of
/// `phases * 2 * taps` doubles per resample call and linear interpolation
/// between adjacent phase rows, which adds error well below the window's
/// own -92 dB sidelobes. 512 phases measured indistinguishable from direct
/// evaluation in the quality test (RMS error against an ideal tone).
class KernelTable {
public:
    static constexpr int phases = 512;
    static constexpr int width  = 2 * taps;   ///< taps per row

    KernelTable(double cutoff)
        : weights_(static_cast<std::size_t>(phases + 1) * width)
    {
        for (int phase = 0; phase <= phases; ++phase) {
            const double fraction = static_cast<double>(phase) / phases;

            for (int tap = 0; tap < width; ++tap) {
                const double offset = static_cast<double>(tap - taps + 1) - fraction;
                weights_[static_cast<std::size_t>(phase) * width + static_cast<std::size_t>(tap)] =
                    cutoff * sinc(cutoff * offset) * window(offset / static_cast<double>(taps));
            }
        }
    }

    /// Row pair for a fractional position, blended by the caller.
    [[nodiscard]] const double* row(int phase) const noexcept
    {
        return weights_.data() + static_cast<std::size_t>(phase) * width;
    }

private:
    std::vector<double> weights_;
};

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

    const KernelTable table(cutoff);

    for (std::size_t channel = 0; channel < source.channelCount; ++channel) {
        const std::vector<Sample>& input  = source.channels[channel];
        std::vector<Sample>&       output = result.channels[channel];

        for (FrameCount frame = 0; frame < outputFrames; ++frame) {
            const double position = static_cast<double>(frame) * ratio;
            const auto   centre   = static_cast<FrameCount>(std::floor(position));
            const double fraction = position - static_cast<double>(centre);

            // The two table rows bracketing this fraction, blended linearly.
            const double phasePosition = fraction * KernelTable::phases;
            const int    phase         = static_cast<int>(phasePosition);
            const double blend         = phasePosition - phase;

            const double* rowLow  = table.row(phase);
            const double* rowHigh = table.row(phase + 1);

            const FrameCount first = centre - taps + 1;
            const FrameCount begin = std::max<FrameCount>(0, first);
            const FrameCount end   = std::min<FrameCount>(centre + taps + 1, source.frameCount);

            double sum = 0.0;

            for (FrameCount index = begin; index < end; ++index) {
                const auto   tap    = static_cast<std::size_t>(index - first);
                const double weight = rowLow[tap] + (rowHigh[tap] - rowLow[tap]) * blend;

                sum += static_cast<double>(input[static_cast<std::size_t>(index)]) * weight;
            }

            output[static_cast<std::size_t>(frame)] = static_cast<Sample>(sum);
        }
    }

    return result;
}

} // namespace incdaw::engine::dsp
