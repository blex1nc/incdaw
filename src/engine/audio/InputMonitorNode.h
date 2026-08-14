#pragma once

#include "engine/core/SampleRingBuffer.h"
#include "engine/graph/Node.h"

#include <vector>

namespace incdaw::engine {

/// Plays the live input: drains the engine's monitor ring into the graph.
///
/// The ring is the bridge between two clock domains — the input device's
/// callback writes it, this node reads it on the output device's thread —
/// and the two genuinely drift. The node caps the drift: when the ring
/// backs up past a few blocks it skips ahead rather than letting monitoring
/// latency grow without bound, and when the input falls momentarily behind
/// it plays silence rather than waiting. Both are counted.
///
/// This is graph monitoring, not direct monitoring: the path is input
/// latency + one ring hop + output latency. Honest, mixable, effectable —
/// and never lower-latency than the buffer sizes allow. A direct hardware
/// monitoring mode would be a device-layer feature, not a node.
class InputMonitorNode final : public Node {
public:
    /// `ring` carries interleaved frames of `channelCount` and must outlive
    /// every graph holding this node — the engine owns it for its own
    /// lifetime, which is what makes the raw pointer safe.
    InputMonitorNode(SampleRingBuffer* ring, std::size_t channelCount) noexcept
        : ring_(ring), channelCount_(channelCount > 0 ? channelCount : 1) {}

    void prepare(SampleRate, FrameCount maxBlockSize) override
    {
        scratch_.assign(static_cast<std::size_t>(maxBlockSize) * channelCount_, 0.0f);
    }

    void process(const ProcessContext& context) noexcept override
    {
        if (ring_ == nullptr || scratch_.empty())
            return;

        const std::size_t want =
            static_cast<std::size_t>(context.frameCount) * channelCount_;

        if (want > scratch_.size())
            return;   // prepare() was not called with a block this big

        // Drift cap: reading at most `want` per block, the backlog only ever
        // grows; past four blocks' worth, skip ahead. Bounded passes — this
        // runs on the audio thread and "catch up" must not mean "spin".
        for (int pass = 0; pass < 8 && ring_->size() > want * 4; ++pass)
            (void)ring_->read(scratch_.data(), want);

        const std::size_t got    = ring_->read(scratch_.data(), want);
        const FrameCount  frames = static_cast<FrameCount>(got / channelCount_);

        for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel) {
            // Mono input on every output channel; extra outputs repeat the
            // last input channel — the recorder's mapping, kept identical so
            // what you monitor is what lands in the take.
            const std::size_t source =
                channel < channelCount_ ? channel : channelCount_ - 1;

            Sample* out = context.output.channel(channel);

            for (FrameCount frame = 0; frame < frames; ++frame)
                out[frame] = scratch_[static_cast<std::size_t>(frame) * channelCount_ + source];
        }
    }

    [[nodiscard]] const char* name() const noexcept override { return "InputMonitor"; }

private:
    SampleRingBuffer*   ring_;
    std::size_t         channelCount_;
    std::vector<Sample> scratch_;
};

} // namespace incdaw::engine
