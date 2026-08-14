#pragma once

#include "engine/graph/Node.h"

#include <atomic>

namespace incdaw::engine::dsp {

/// Sums its inputs and applies a gain.
///
/// Also the graph's summing point: a node with several sources receives them as
/// separate input views and is responsible for combining them. With unity gain
/// and no sources this is silence; with unity gain and one source it is a
/// pass-through.
///
/// The gain is smoothed across the block rather than applied as a step. An
/// instantaneous gain change produces a discontinuity in the waveform, which is
/// audible as a click — the single most common artefact in a naive mixer.
class GainNode final : public Node {
public:
    explicit GainNode(Sample gain = Sample{1}) noexcept : target_(gain), current_(gain) {}

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override
    {
        (void)maxBlockSize;
        sampleRate_ = sampleRate;
        current_    = target_.load(std::memory_order_relaxed);
    }

    void process(const ProcessContext& context) noexcept override;

    void setGain(Sample gain) noexcept { target_.store(gain, std::memory_order_relaxed); }
    [[nodiscard]] Sample gain() const noexcept { return target_.load(std::memory_order_relaxed); }

    /// The gain actually reached at the end of the last block. Differs from
    /// `gain()` while a change is still being smoothed in.
    [[nodiscard]] Sample currentGain() const noexcept { return current_; }

    [[nodiscard]] const char* name() const noexcept override { return "Gain"; }

private:
    std::atomic<Sample> target_{Sample{1}};
    Sample              current_    = Sample{1};
    SampleRate          sampleRate_ = 0.0;
};

} // namespace incdaw::engine::dsp
