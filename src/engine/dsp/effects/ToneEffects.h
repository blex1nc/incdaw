#pragma once

#include "engine/dsp/effects/BuiltinEffect.h"

namespace incdaw::engine::dsp {

/// A state-variable filter as an insert: off, lowpass, highpass or bandpass.
/// Off is bit-exact bypass, which is also the null test.
class FilterEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t { mode = 0, cutoffHz = 1, resonance = 2 };
    enum Mode : int { off = 0, lowpass = 1, highpass = 2, bandpass = 3 };

    static constexpr std::size_t maxChannels = 8;

    FilterEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Filter"; }

private:
    struct State {
        double low  = 0.0;
        double band = 0.0;
    };

    State      states_[maxChannels]{};
    SampleRate sampleRate_ = 48000.0;
};

/// Three-band parametric EQ: low shelf, peak, high shelf — RBJ cookbook
/// biquads. Every band at 0 dB is coefficient-identity, so the defaulted EQ
/// nulls against its input.
class EqEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t {
        lowFreq   = 0,
        lowGainDb = 1,
        midFreq   = 2,
        midGainDb = 3,
        midQ      = 4,
        highFreq   = 5,
        highGainDb = 6,
    };

    static constexpr std::size_t maxChannels = 8;
    static constexpr std::size_t bandCount   = 3;

    EqEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "EQ"; }

private:
    struct Coefficients {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    };

    struct State {
        double z1 = 0.0, z2 = 0.0;   ///< transposed direct form II
    };

    void updateCoefficients() noexcept;

    Coefficients coefficients_[bandCount];
    State        states_[bandCount][maxChannels]{};
    double       cached_[7] = {-1, -1, -1, -1, -1, -1, -1};
    SampleRate   sampleRate_ = 48000.0;
};

/// tanh waveshaper with a dry/wet mix. Drive 0 dB is an explicit bypass —
/// the effect's transparency claim is structural, not numeric.
class SaturatorEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t { driveDb = 0, mix = 1 };

    SaturatorEffect();

    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Saturator"; }
};

} // namespace incdaw::engine::dsp
