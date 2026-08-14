#include "engine/dsp/DelayLineNode.h"

namespace incdaw::engine::dsp {

void DelayLineNode::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)sampleRate;

    // Sized for the worst case the graph can hand this node, so `process` never
    // has to consider growing. Two channels is what the graph compiles for
    // today; a wider graph re-prepares.
    channelCount_ = 2;
    capacity_     = delayFrames_ + (maxBlockSize > 0 ? maxBlockSize : 512) + 1;
    writeIndex_   = 0;

    history_.assign(static_cast<std::size_t>(capacity_) * channelCount_, Sample{0});
}

void DelayLineNode::process(const ProcessContext& context) noexcept
{
    for (std::size_t index = 0; index < context.inputCount; ++index)
        context.output.addFrom(context.input(index));

    if (delayFrames_ <= 0 || history_.empty())
        return;

    const std::size_t channels = context.output.channelCount() < channelCount_
                                     ? context.output.channelCount()
                                     : channelCount_;

    for (std::size_t channel = 0; channel < channels; ++channel) {
        Sample*     samples = context.output.channel(channel);
        Sample*     line    = history_.data() + static_cast<std::size_t>(capacity_) * channel;
        FrameCount  write   = writeIndex_;

        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            // Read the sample written `delayFrames_` ago, then overwrite that
            // slot with the incoming one: one pass, no second buffer.
            FrameCount read = write - delayFrames_;
            while (read < 0)
                read += capacity_;

            const Sample delayed = line[read];
            line[write] = samples[frame];
            samples[frame] = delayed;

            if (++write >= capacity_)
                write = 0;
        }
    }

    writeIndex_ = (writeIndex_ + context.frameCount) % capacity_;
}

} // namespace incdaw::engine::dsp
