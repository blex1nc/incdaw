#include "engine/dsp/effects/UtilityEffects.h"

#include <cmath>

namespace incdaw::engine::dsp {

namespace {

constexpr EffectParameter utilityParameters[] = {
    {UtilityEffect::gainDb,   "Gain",     -60.0, 24.0, 0.0, false},
    {UtilityEffect::pan,      "Pan",       -1.0,  1.0, 0.0, false},
    {UtilityEffect::width,    "Width",      0.0,  2.0, 1.0, false},
    {UtilityEffect::polarity, "Polarity",   0.0,  1.0, 0.0, true},
    {UtilityEffect::mono,     "Mono",       0.0,  1.0, 0.0, true},
};

} // namespace

UtilityEffect::UtilityEffect() : BuiltinEffect(utilityParameters, 5) {}

void UtilityEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double gain     = dbToGain(valueAt(0)) * (valueAt(3) >= 0.5 ? -1.0 : 1.0);
    const double panValue = valueAt(1);
    const double widthVal = valueAt(2);
    const bool   toMono   = valueAt(4) >= 0.5;

    const std::size_t channels = context.output.channelCount();
    const FrameCount  frames   = context.frameCount;

    // Constant-power balance, normalised against its centre value so that
    // pan 0 is exactly unity — the null test holds the defaults to that.
    double panLeft  = 1.0;
    double panRight = 1.0;
    if (panValue != 0.0) {
        constexpr double quarterPi = 0.78539816339744830962;
        panLeft  = std::cos((panValue + 1.0) * quarterPi / 2.0) / std::cos(quarterPi / 2.0);
        panRight = std::sin((panValue + 1.0) * quarterPi / 2.0) / std::sin(quarterPi / 2.0);
    }

    if (channels >= 2) {
        Sample* left  = context.output.channel(0);
        Sample* right = context.output.channel(1);

        for (FrameCount frame = 0; frame < frames; ++frame) {
            double sampleLeft  = static_cast<double>(left[frame]);
            double sampleRight = static_cast<double>(right[frame]);

            if (toMono) {
                const double centre = (sampleLeft + sampleRight) * 0.5;
                sampleLeft = sampleRight = centre;
            } else if (widthVal != 1.0) {
                const double mid  = (sampleLeft + sampleRight) * 0.5;
                const double side = (sampleLeft - sampleRight) * 0.5 * widthVal;
                sampleLeft  = mid + side;
                sampleRight = mid - side;
            }

            left[frame]  = static_cast<Sample>(sampleLeft * gain * panLeft);
            right[frame] = static_cast<Sample>(sampleRight * gain * panRight);
        }

        // Any further channels get plain gain — width and pan are a stereo idea.
        for (std::size_t channel = 2; channel < channels; ++channel) {
            Sample* samples = context.output.channel(channel);
            for (FrameCount frame = 0; frame < frames; ++frame)
                samples[frame] = static_cast<Sample>(static_cast<double>(samples[frame]) * gain);
        }

        return;
    }

    for (std::size_t channel = 0; channel < channels; ++channel) {
        Sample* samples = context.output.channel(channel);
        for (FrameCount frame = 0; frame < frames; ++frame)
            samples[frame] = static_cast<Sample>(static_cast<double>(samples[frame]) * gain);
    }
}

AnalyzerEffect::AnalyzerEffect() : BuiltinEffect(nullptr, 0) {}

void AnalyzerEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const std::size_t channels =
        context.output.channelCount() < maxChannels ? context.output.channelCount() : maxChannels;

    for (std::size_t channel = 0; channel < channels; ++channel) {
        const Sample* samples = context.output.channel(channel);

        float  peak = 0.0f;
        double sum  = 0.0;
        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            const float value = std::fabs(samples[frame]);
            peak = value > peak ? value : peak;
            sum += static_cast<double>(samples[frame]) * static_cast<double>(samples[frame]);
        }

        peak_[channel].store(peak, std::memory_order_relaxed);
        rms_[channel].store(
            context.frameCount > 0
                ? static_cast<float>(std::sqrt(sum / static_cast<double>(context.frameCount)))
                : 0.0f,
            std::memory_order_relaxed);
    }
}

} // namespace incdaw::engine::dsp
