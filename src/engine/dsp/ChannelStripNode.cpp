#include "engine/dsp/ChannelStripNode.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine::dsp {
namespace {

constexpr double smoothingSeconds = 0.005;
constexpr double halfPi           = 1.57079632679489661923;

/// Constant-power pan law. -1 is hard left, 0 centre, +1 hard right; a centred
/// signal sits at -3 dB in each channel, which is what keeps the perceived
/// loudness flat as it moves across the field.
void panGains(Sample pan, Sample& left, Sample& right) noexcept
{
    const double position = (std::clamp(static_cast<double>(pan), -1.0, 1.0) + 1.0) * 0.5;
    left  = static_cast<Sample>(std::cos(position * halfPi));
    right = static_cast<Sample>(std::sin(position * halfPi));
}

} // namespace

void ChannelStripNode::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate;
    primed_     = false;   // start at the target rather than sliding up to it
}

void ChannelStripNode::process(const ProcessContext& context) noexcept
{
    for (std::size_t index = 0; index < context.inputCount; ++index)
        context.output.addFrom(context.input(index));

    const Sample volume = muted_.load(std::memory_order_relaxed)
                              ? Sample{0}
                              : volume_.load(std::memory_order_relaxed);

    Sample panLeft = Sample{1};
    Sample panRight = Sample{1};
    panGains(pan_.load(std::memory_order_relaxed), panLeft, panRight);

    // Mono output gets the summed pan gain, so that panning a channel on a mono
    // device changes nothing but its level, rather than silencing it.
    const bool   stereo      = context.output.channelCount() >= 2;
    const Sample targetLeft  = stereo ? volume * panLeft : volume;
    const Sample targetRight = stereo ? volume * panRight : volume;

    if (!primed_) {
        currentLeft_  = targetLeft;
        currentRight_ = targetRight;
        primed_       = true;
    }

    const SampleRate rate = context.sampleRate > 0.0 ? context.sampleRate : sampleRate_;
    const Sample coefficient = rate > 0.0
        ? static_cast<Sample>(1.0 - std::exp(-1.0 / (smoothingSeconds * rate)))
        : Sample{1};

    Sample peak = Sample{0};

    // Captured before the loop: every left channel must start its smoother
    // from the same state, or a four-channel buffer would ramp differently in
    // channel 2 than in channel 0.
    const Sample startLeft  = currentLeft_;
    const Sample startRight = currentRight_;

    for (std::size_t channelIndex = 0; channelIndex < context.output.channelCount(); ++channelIndex) {
        Sample* samples = context.output.channel(channelIndex);

        const bool   isRight = (channelIndex % 2) == 1;
        const Sample target  = isRight ? targetRight : targetLeft;
        Sample       value   = isRight ? startRight : startLeft;

        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            value += (target - value) * coefficient;
            samples[frame] *= value;

            const Sample magnitude = samples[frame] < Sample{0} ? -samples[frame] : samples[frame];
            peak = magnitude > peak ? magnitude : peak;
        }

        // Left and right advance their own smoothers; committing inside the
        // loop would let a later channel read a state the earlier one moved.
        if (isRight)
            currentRight_ = value;
        else
            currentLeft_ = value;
    }

    peak_.store(peak, std::memory_order_relaxed);
}

} // namespace incdaw::engine::dsp
