#include "engine/dsp/MixerStripNode.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine::dsp {

void MixerStripNode::panGains(double pan, Sample& left, Sample& right) noexcept
{
    const double clamped = std::clamp(pan, -1.0, 1.0);

    // Constant power: the two gains are the cosine and sine of one angle, so
    // their squares always sum to one. Centre lands on 1/sqrt(2), which is the
    // -3 dB the ear expects.
    const double angle = (clamped + 1.0) * 0.25 * 3.14159265358979323846;

    left  = static_cast<Sample>(std::cos(angle));
    right = static_cast<Sample>(std::sin(angle));
}

void MixerStripNode::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;

    left_.prepare(sampleRate);
    right_.prepare(sampleRate);
    meter_.prepare(sampleRate);

    refreshTargets();

    // Starting mid-ramp would fade the first block in from whatever the
    // previous graph left behind.
    left_.snap();
    right_.snap();
}

void MixerStripNode::refreshTargets() noexcept
{
    Sample panLeft = 1.0f;
    Sample panRight = 1.0f;
    panGains(pan_.load(std::memory_order_relaxed), panLeft, panRight);

    const Sample gain = muted_.load(std::memory_order_relaxed)
                            ? Sample{0}
                            : gain_.load(std::memory_order_relaxed);

    // Polarity folds into the gain rather than being a separate pass: it is a
    // multiplication by -1, and smoothing through zero is exactly the
    // click-free behaviour flipping it should have.
    const Sample sign = polarityInverted_.load(std::memory_order_relaxed) ? Sample{-1} : Sample{1};

    left_.setTarget(gain * panLeft * sign);
    right_.setTarget(gain * panRight * sign);
}

void MixerStripNode::setGain(Sample gain) noexcept
{
    gain_.store(gain, std::memory_order_relaxed);
    refreshTargets();
}

void MixerStripNode::setPan(double pan) noexcept
{
    pan_.store(pan, std::memory_order_relaxed);
    refreshTargets();
}

void MixerStripNode::setMuted(bool muted) noexcept
{
    muted_.store(muted, std::memory_order_relaxed);
    refreshTargets();
}

void MixerStripNode::setPolarityInverted(bool inverted) noexcept
{
    polarityInverted_.store(inverted, std::memory_order_relaxed);
    refreshTargets();
}

void MixerStripNode::process(const ProcessContext& context) noexcept
{
    // Summing point. `output` arrives silenced, so a strip with no sources
    // correctly produces silence rather than the previous block's contents.
    for (std::size_t index = 0; index < context.inputCount; ++index)
        context.output.addFrom(context.input(index));

    const std::size_t channels = context.output.channelCount();

    if (channels > 0)
        left_.applyToChannel(context.output.channel(0), context.frameCount, true);

    if (channels > 1)
        right_.applyToChannel(context.output.channel(1), context.frameCount, true);

    // Beyond stereo the pan law has nothing to say yet, so those channels get
    // the fader only — via the left ramp's value, without advancing it again.
    for (std::size_t channel = 2; channel < channels; ++channel) {
        Sample* samples = context.output.channel(channel);
        const Sample value = left_.current();

        for (FrameCount frame = 0; frame < context.frameCount; ++frame)
            samples[frame] *= value;
    }

    meter_.measure(context.output, context.frameCount);
}

} // namespace incdaw::engine::dsp
