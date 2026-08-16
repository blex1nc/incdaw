#pragma once

#include "engine/dsp/effects/BuiltinEffect.h"

namespace incdaw::engine::dsp {

/// Feed-forward compressor: linked peak detector, one-pole attack/release
/// smoothing on the gain, log-domain gain computer, makeup. Ratio 1 with no
/// makeup is exactly unity — the null test.
class CompressorEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t {
        thresholdDb = 0,
        ratio       = 1,
        attackMs    = 2,
        releaseMs   = 3,
        makeupDb    = 4,
    };

    CompressorEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Compressor"; }

    /// Gain reduction of the last block in dB, for the UI's meter.
    [[nodiscard]] double gainReductionDb() const noexcept
    {
        return reduction_.load(std::memory_order_relaxed);
    }

    static constexpr std::size_t noKeyInput = static_cast<std::size_t>(-1);

    /// Marks which graph input feeds the detector instead of the audio path —
    /// external sidechain. Set by the graph compiler when a sidechain edge
    /// lands on this insert; build time only, before the node ever renders.
    void setKeyInput(std::size_t index) noexcept { keyInput_ = index; }
    [[nodiscard]] std::size_t keyInput() const noexcept { return keyInput_; }

private:
    double              envelope_ = 1.0;   ///< smoothed gain, linear
    SampleRate          sampleRate_ = 48000.0;
    std::size_t         keyInput_   = noKeyInput;
    std::atomic<double> reduction_{0.0};
};

/// Zero-lookahead peak limiter: instant attack to keep the ceiling, one-pole
/// release back to unity. A signal already under the ceiling passes bit-exact.
class LimiterEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t { ceilingDb = 0, releaseMs = 1 };

    LimiterEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Limiter"; }

private:
    double     gain_       = 1.0;
    SampleRate sampleRate_ = 48000.0;
};

/// Downward gate: linked peak detector opens at threshold, holds, then
/// releases. At the floor threshold the gate is always open — unity.
class GateEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t {
        thresholdDb = 0,
        attackMs    = 1,
        holdMs      = 2,
        releaseMs   = 3,
    };

    GateEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Gate"; }

private:
    double     gain_       = 1.0;   ///< smoothed open/closed gain
    double     holdLeft_   = 0.0;   ///< frames of hold remaining
    SampleRate sampleRate_ = 48000.0;
};

} // namespace incdaw::engine::dsp
