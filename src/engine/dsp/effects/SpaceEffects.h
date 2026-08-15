#pragma once

#include "engine/dsp/effects/BuiltinEffect.h"

#include <vector>

namespace incdaw::engine::dsp {

/// A feedback delay with a dry/wet mix. The line is sized for the longest
/// time in prepare; a time change moves the read tap, never reallocates.
/// Mix 0 passes the dry signal bit-exact.
class DelayEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t { timeMs = 0, feedback = 1, mix = 2 };

    static constexpr std::size_t maxChannels = 2;
    static constexpr double      maxTimeMs   = 2000.0;

    DelayEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Delay"; }

private:
    std::vector<Sample> lines_[maxChannels];
    FrameCount          capacity_   = 0;
    FrameCount          writeIndex_ = 0;
    SampleRate          sampleRate_ = 48000.0;
};

/// A Schroeder reverberator: four parallel damped combs into two series
/// allpasses per channel, the right channel's line lengths offset for
/// stereo decorrelation. An independent implementation of the classic
/// public-domain topology. Mix 0 passes the dry signal bit-exact.
class ReverbEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t { size = 0, damping = 1, mix = 2 };

    static constexpr std::size_t maxChannels   = 2;
    static constexpr std::size_t combCount     = 4;
    static constexpr std::size_t allpassCount  = 2;

    ReverbEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Reverb"; }

private:
    struct Comb {
        std::vector<Sample> line;
        std::size_t         index = 0;
        double              store = 0.0;   ///< damping lowpass state
    };

    struct Allpass {
        std::vector<Sample> line;
        std::size_t         index = 0;
    };

    Comb    combs_[maxChannels][combCount];
    Allpass allpasses_[maxChannels][allpassCount];
};

} // namespace incdaw::engine::dsp
