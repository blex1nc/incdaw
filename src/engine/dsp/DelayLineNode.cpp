#include "engine/dsp/DelayLineNode.h"

#include <algorithm>

namespace incdaw::engine::dsp {

void DelayLineNode::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)sampleRate;

    // The ring holds the delay plus one block, so a block can be written and
    // read in the same pass without the write overtaking the read.
    ringLength_ = static_cast<std::size_t>(frames_ + std::max<FrameCount>(1, maxBlockSize));
    writeIndex_ = 0;

    // Zeroed, not merely sized: the first `frames_` samples this node emits come
    // out of the ring before anything has been written into it, and whatever the
    // allocator handed us would otherwise be audible as a click.
    history_.assign(channels_ * ringLength_, Sample{0});
}

void DelayLineNode::process(const ProcessContext& context) noexcept
{
    for (std::size_t index = 0; index < context.inputCount; ++index)
        context.output.addFrom(context.input(index));

    if (frames_ <= 0 || context.frameCount <= 0)
        return;

    // A block wider than the ring was allocated for passes through undelayed
    // rather than reallocating on the audio thread. It cannot happen once the
    // graph and the device agree on a width; if it ever does, an unexpectedly
    // dry channel is a far better failure than a dropout.
    const std::size_t channels = std::min(context.output.channelCount(), channels_);

    if (history_.empty() || ringLength_ == 0 || channels == 0)
        return;

    for (std::size_t channel = 0; channel < channels; ++channel) {
        Sample*     samples = context.output.channel(channel);
        Sample*     ring    = history_.data() + channel * ringLength_;
        std::size_t write   = writeIndex_;

        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            const std::size_t read = (write + ringLength_ - static_cast<std::size_t>(frames_)) % ringLength_;

            const Sample delayed = ring[read];
            ring[write]  = samples[frame];
            samples[frame] = delayed;

            write = (write + 1) % ringLength_;
        }

        // Every channel walks the same ring positions; the index is committed
        // once, after the last one.
        if (channel + 1 == channels)
            writeIndex_ = write;
    }
}

} // namespace incdaw::engine::dsp
