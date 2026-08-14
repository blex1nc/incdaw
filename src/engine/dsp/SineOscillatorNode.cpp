#include "engine/dsp/SineOscillatorNode.h"

#include <cmath>
#include <numbers>

namespace incdaw::engine::dsp {

void SineOscillatorNode::process(const ProcessContext& context) noexcept
{
    const SampleRate rate = context.sampleRate > 0.0 ? context.sampleRate : sampleRate_;
    if (rate <= 0.0 || context.output.channelCount() == 0)
        return;

    const double frequency = frequency_.load(std::memory_order_relaxed);
    const Sample amplitude = amplitude_.load(std::memory_order_relaxed);
    const double increment = frequency / rate;

    // Generate once into channel 0, then copy: the sine call is the expensive
    // part, and a mono source feeding N channels should pay for it once.
    Sample* first = context.output.channel(0);
    double  phase = phase_;

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        first[frame] = amplitude * static_cast<Sample>(std::sin(2.0 * std::numbers::pi * phase));

        phase += increment;
        if (phase >= 1.0)
            phase -= std::floor(phase);   // exact wrap; no radian drift
    }

    phase_ = phase;

    for (std::size_t channelIndex = 1; channelIndex < context.output.channelCount(); ++channelIndex) {
        Sample* samples = context.output.channel(channelIndex);
        for (FrameCount frame = 0; frame < context.frameCount; ++frame)
            samples[frame] = first[frame];
    }
}

} // namespace incdaw::engine::dsp
