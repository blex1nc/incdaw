#pragma once

#include "engine/core/AudioBuffer.h"
#include "engine/graph/Node.h"

#include <vector>

namespace incdaw::engine::dsp {

/// A fixed whole-frame delay that declares itself.
///
/// Two jobs, deliberately the same object. It is what delay compensation
/// inserts on the short side of a summing point, and it is what an effect with
/// look-ahead behaves like from the graph's point of view. A node that delayed
/// its signal without reporting `latencyFrames` would silently smear the mix;
/// one that reported latency without delaying anything would make the compiler
/// compensate for a delay that never happened.
///
/// Whole frames only. Fractional-delay compensation belongs to the resampling
/// path, not here (docs/AUDIO_ENGINE.md §7).
class DelayLineNode final : public Node {
public:
    /// `channels` is how wide the ring is allocated. It is a constructor
    /// argument rather than something learned in `prepare` because `prepare`
    /// is not told the graph's width, and learning it from the first block
    /// would mean allocating on the audio thread.
    explicit DelayLineNode(FrameCount frames, std::size_t channels = 2) noexcept
        : frames_(frames > 0 ? frames : 0), channels_(channels > 0 ? channels : 1) {}

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] FrameCount latencyFrames() const noexcept override { return frames_; }
    [[nodiscard]] const char* name() const noexcept override { return "Delay"; }

private:
    FrameCount          frames_ = 0;
    std::size_t         channels_ = 2;
    std::size_t         writeIndex_ = 0;

    /// One contiguous ring per channel, laid out end to end. Allocated in
    /// `prepare`, never in `process`.
    std::vector<Sample> history_;
    std::size_t         ringLength_ = 0;
};

} // namespace incdaw::engine::dsp
