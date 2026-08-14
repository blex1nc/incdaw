#pragma once

#include "engine/graph/Node.h"

#include <vector>

namespace incdaw::engine::dsp {

/// A fixed whole-sample delay.
///
/// The compensating element of plugin delay compensation: the graph inserts one
/// of these on every path that is shorter than the longest path into a summing
/// node, so that everything arrives together (docs/AUDIO_ENGINE.md §7).
///
/// The delay is fixed for the life of the node. Changing it means recompiling
/// the graph, which is what happens anyway when a plugin reports new latency —
/// and a delay line that could be resized would have to allocate on the audio
/// thread to do it.
class DelayLineNode final : public Node {
public:
    explicit DelayLineNode(FrameCount delayFrames) noexcept
        : delayFrames_(delayFrames > 0 ? delayFrames : 0) {}

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    /// This node's own contribution to the graph's latency — which is exactly
    /// the delay it applies. Reporting it is what keeps the compiler's
    /// arithmetic consistent when a delay line feeds another summing point.
    [[nodiscard]] FrameCount latencyFrames() const noexcept override { return delayFrames_; }

    [[nodiscard]] const char* name() const noexcept override { return "Delay"; }

private:
    FrameCount          delayFrames_ = 0;
    std::size_t         channelCount_ = 0;
    FrameCount          capacity_    = 0;
    FrameCount          writeIndex_  = 0;
    std::vector<Sample> history_;   ///< channelCount_ * capacity_, allocated in prepare
};

} // namespace incdaw::engine::dsp
