#pragma once

#include "engine/dsp/effects/BuiltinEffect.h"

#include <cstddef>
#include <vector>

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

/// Lookahead peak limiter: the signal is delayed by a fixed two-millisecond
/// window and the gain is computed against the window's PEAK, so attenuation
/// arrives before the transient does — the ceiling holds without the
/// instant-attack grit of the zero-lookahead limiter.
///
/// The window is fixed, not a parameter, deliberately: latency is reported
/// to the graph at CONSTRUCTION (topology reads it before prepare), and a
/// latency that moved with a knob would leave delay compensation stale
/// between rebuilds. Two milliseconds is the conventional transparent
/// figure. A separate catalogued effect rather than a mode of the classic
/// limiter, so the classic's node-level bit-exact null (Phase 15's exit
/// criterion) stays untouched; this one's transparency claim is "a pure
/// delay of its window", which delay compensation absorbs in the graph.
class LookaheadLimiterEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t { ceilingDb = 0, releaseMs = 1 };

    /// Milliseconds of lookahead — public so the tests and the latency
    /// assertion agree with the implementation by construction.
    static constexpr double lookaheadMilliseconds = 2.0;

    explicit LookaheadLimiterEffect(SampleRate sampleRate);

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Limiter (Lookahead)"; }
    [[nodiscard]] FrameCount latencyFrames() const noexcept override { return lookahead_; }

private:
    FrameCount lookahead_  = 0;
    SampleRate sampleRate_ = 48000.0;
    double     gain_       = 1.0;

    /// The delay line (stereo) and the sliding-maximum machinery, all sized
    /// in prepare — process never allocates.
    std::vector<double> delay_[2];
    std::size_t         writeIndex_ = 0;

    /// Monotonic deque over the window's linked peaks: front is the maximum.
    /// Ring-buffered indices; capacity lookahead_ + 1.
    std::vector<double>     windowPeaks_;
    std::vector<FrameCount> dequeFrames_;
    std::vector<double>     dequeValues_;
    std::size_t             dequeHead_ = 0;
    std::size_t             dequeTail_ = 0;
    FrameCount              frameClock_ = 0;
};

/// Splits the signal into transient and sustain components and rebalances
/// them — the functional core of FL Studio 2026's Transmitter. A fast and a
/// slow envelope race: where the fast one leads, the sound is a transient.
/// `output` selects both (rebalanced), transients only, or sustain only, so
/// two parallel strips can each take one half and shape it independently.
/// At its defaults (both, 0 dB, 0 dB) it is exactly unity — the null test.
class TransientSplitEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t {
        output      = 0,   ///< 0 both · 1 transients · 2 sustain
        transientDb = 1,
        sustainDb   = 2,
    };

    enum class Output : int { both = 0, transientsOnly = 1, sustainOnly = 2 };

    TransientSplitEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Transient Split"; }

private:
    double     fast_ = 0.0;   ///< fast envelope, linked across channels
    double     slow_ = 0.0;   ///< slow envelope
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
