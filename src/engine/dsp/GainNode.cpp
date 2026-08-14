#include "engine/dsp/GainNode.h"

#include <cmath>

namespace incdaw::engine::dsp {
namespace {

/// Time constant for gain smoothing. 5 ms is short enough to feel immediate on
/// a fader and long enough to remove the discontinuity that causes clicks.
constexpr double smoothingSeconds = 0.005;

} // namespace

void GainNode::process(const ProcessContext& context) noexcept
{
    // Sum the sources. `output` arrives silenced, so with no sources this
    // correctly produces silence.
    for (std::size_t index = 0; index < context.inputCount; ++index)
        context.output.addFrom(context.input(index));

    const Sample target = target_.load(std::memory_order_relaxed);

    if (current_ == target) {
        if (target != Sample{1})
            context.output.applyGain(target);
        return;
    }

    const SampleRate rate = context.sampleRate > 0.0 ? context.sampleRate : sampleRate_;

    // One-pole smoothing towards the target.
    const Sample coefficient = rate > 0.0
        ? static_cast<Sample>(1.0 - std::exp(-1.0 / (smoothingSeconds * rate)))
        : 1.0f;

    for (std::size_t channelIndex = 0; channelIndex < context.output.channelCount(); ++channelIndex) {
        Sample* samples = context.output.channel(channelIndex);
        Sample  value   = current_;

        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            value += (target - value) * coefficient;
            samples[frame] *= value;
        }

        // Every channel must advance the smoother identically, so the state is
        // only committed once, after the last one.
        if (channelIndex + 1 == context.output.channelCount())
            current_ = value;
    }

    // Snap once the remaining difference is inaudible, so that `current_` does
    // not creep towards the target forever and keep the smoothing path hot.
    const Sample remaining = current_ > target ? current_ - target : target - current_;
    if (remaining < 1e-6f)
        current_ = target;
}

} // namespace incdaw::engine::dsp
