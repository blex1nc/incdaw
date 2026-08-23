#pragma once

// Multiband dynamics (A6) — a compressor per frequency band.
//
// The load-bearing property is the CROSSOVER, not the compressor: three bands
// that do not sum back to the signal they were split from make an effect that
// colours everything it touches whether it is doing anything or not. The
// network here is a serial Linkwitz-Riley tree,
//
//     low  = AP(f2, LP(f1, x))
//     mid  =        LP(f2, HP(f1, x))
//     high =        HP(f2, HP(f1, x))
//
// whose sum is AP(f2, AP(f1, x)) — an allpass, so the magnitude response is
// flat to within the arithmetic. The allpass on the low band is what keeps
// the three phase-aligned; without it the sum has a hole at the second
// crossover.
//
// An allpass is flat but not an identity: the sum is phase-shifted, so it
// does not null against its input sample by sample. The transparency claim is
// therefore STRUCTURAL, exactly as the saturator's is — a multiband doing
// nothing skips the split entirely and passes the signal through untouched.

#include "engine/dsp/effects/BuiltinEffect.h"
#include "engine/dsp/effects/ToneEffects.h"

#include <array>
#include <atomic>
#include <cstddef>

namespace incdaw::engine::dsp {

class MultibandCompressorEffect final : public BuiltinEffect {
public:
    static constexpr std::size_t bandCount   = 3;
    static constexpr std::size_t maxChannels = 8;

    /// Ids are frozen: they key the state blob and every saved preset.
    enum Param : std::uint32_t {
        crossoverLowHz  = 0,
        crossoverHighHz = 1,
        outputDb        = 2,
    };

    /// Per-band ids live at `bandBase + band * bandStride + offset`.
    enum BandOffset : std::uint32_t {
        bandThresholdDb = 0,
        bandRatio       = 1,
        bandAttackMs    = 2,
        bandReleaseMs   = 3,
        bandMakeupDb    = 4,
        bandBypass      = 5,
        bandSolo        = 6,
    };

    static constexpr std::uint32_t bandBase   = 10;
    static constexpr std::uint32_t bandStride = 10;

    [[nodiscard]] static constexpr std::uint32_t bandParameter(std::size_t band,
                                                               BandOffset  offset) noexcept
    {
        return bandBase + static_cast<std::uint32_t>(band) * bandStride
             + static_cast<std::uint32_t>(offset);
    }

    MultibandCompressorEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Multiband Compressor"; }

    /// Gain reduction of the last block, per band, in dB. For the meter.
    [[nodiscard]] double gainReductionDb(std::size_t band) const noexcept
    {
        return band < bandCount ? reduction_[band].load(std::memory_order_relaxed) : 0.0;
    }

private:
    /// One biquad's state, transposed direct form II — the same form the EQ
    /// uses, so the two cannot disagree about what a biquad is.
    struct Section {
        double z1 = 0.0, z2 = 0.0;

        [[nodiscard]] double step(const BiquadCoefficients& c, double input) noexcept
        {
            const double output = c.b0 * input + z1;
            z1 = c.b1 * input - c.a1 * output + z2;
            z2 = c.b2 * input - c.a2 * output;
            return output;
        }
    };

    /// A fourth-order Linkwitz-Riley half: two identical Butterworth sections
    /// in series.
    struct Pair {
        Section first, second;

        [[nodiscard]] double step(const BiquadCoefficients& c, double input) noexcept
        {
            return second.step(c, first.step(c, input));
        }

        void reset() noexcept { first = {}; second = {}; }
    };

    struct ChannelState {
        Pair lowSplit, highSplit;         ///< the split at the first crossover
        Pair midSplit, topSplit;          ///< the second crossover, on the high branch
        Pair lowAllpassLow, lowAllpassHigh;   ///< the second crossover again, on the low band

        void reset() noexcept
        {
            lowSplit.reset();
            highSplit.reset();
            midSplit.reset();
            topSplit.reset();
            lowAllpassLow.reset();
            lowAllpassHigh.reset();
        }
    };

    void designCrossovers(double lowHz, double highHz) noexcept;

    BiquadCoefficients lowpassLow_{}, highpassLow_{};
    BiquadCoefficients lowpassHigh_{}, highpassHigh_{};

    double cachedLowHz_  = -1.0;
    double cachedHighHz_ = -1.0;

    std::array<ChannelState, maxChannels> channels_{};
    std::array<double, bandCount>         envelope_{};

    SampleRate                                     sampleRate_ = 48000.0;
    std::array<std::atomic<double>, bandCount>     reduction_{};
};

/// Splits the three-band network's crossover response for a UI or a test:
/// the summed magnitude of the network at `frequency`, in dB, designed with
/// the same code the audio thread runs.
[[nodiscard]] double multibandSumMagnitudeDb(double lowHz, double highHz,
                                             SampleRate sampleRate, double frequency) noexcept;

} // namespace incdaw::engine::dsp
