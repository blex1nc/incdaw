#pragma once

// Stereo imaging (A8) — width, per band, with a picture of what it did.
//
// A mixer strip already has a stereo-separation control, and this is not a
// second one of those: strip separation is one number for the whole signal,
// which is the wrong tool almost every time. Bass wants to be mono and the
// top wants to be wide, and a single width knob cannot say both.
//
// So: three bands over the shared Linkwitz-Riley tree, a width per band, and
// a mono-below frequency for the case where all that is wanted is a centred
// bottom end. The correlation meter is the picture — the number that says
// whether what has just been widened will survive a mono fold-down.
//
// Width is mid/side: side is scaled, mid is not. Width 1 is exactly the
// input, width 0 is mono, width 2 is side doubled. At width 1 on every band,
// no mono-below and unity output, the effect skips its split entirely and
// passes the signal through bit-exact — the split's sum is an allpass, which
// is flat but not an identity.

#include "engine/dsp/effects/BuiltinEffect.h"
#include "engine/dsp/effects/Crossover.h"

#include <array>
#include <atomic>
#include <cstddef>

namespace incdaw::engine::dsp {

class StereoImagerEffect final : public BuiltinEffect {
public:
    static constexpr std::size_t bandCount = 3;

    /// Ids are frozen: they key the state blob and every saved preset.
    enum Param : std::uint32_t {
        lowWidth        = 0,
        midWidth        = 1,
        highWidth       = 2,
        crossoverLowHz  = 3,
        crossoverHighHz = 4,
        monoBelowHz     = 5,   ///< 0 turns it off
        outputDb        = 6,
    };

    StereoImagerEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Stereo Imager"; }

    /// Pearson correlation of the last block, -1 (out of phase) to +1 (mono).
    /// Read from any thread.
    [[nodiscard]] double correlation() const noexcept
    {
        return correlation_.load(std::memory_order_relaxed);
    }

private:
    struct ChannelState {
        LinkwitzRileyHalf lowSplit, highSplit;
        LinkwitzRileyHalf midSplit, topSplit;
        LinkwitzRileyHalf lowAllpassLow, lowAllpassHigh;

        LinkwitzRileyHalf monoLow, monoHigh;

        void reset() noexcept
        {
            lowSplit.reset();
            highSplit.reset();
            midSplit.reset();
            topSplit.reset();
            lowAllpassLow.reset();
            lowAllpassHigh.reset();
            monoLow.reset();
            monoHigh.reset();
        }
    };

    void designFor(double lowHz, double highHz, double monoHz) noexcept;

    BiquadCoefficients lowpassLow_{}, highpassLow_{};
    BiquadCoefficients lowpassHigh_{}, highpassHigh_{};
    BiquadCoefficients lowpassMono_{}, highpassMono_{};

    double cachedLowHz_  = -1.0;
    double cachedHighHz_ = -1.0;
    double cachedMonoHz_ = -1.0;

    /// Stereo only. A mono signal has no side to scale, and a surround one
    /// would need a different question answered first.
    static constexpr std::size_t channelCount = 2;

    std::array<ChannelState, channelCount> channels_{};

    SampleRate          sampleRate_ = 48000.0;
    std::atomic<double> correlation_{1.0};
};

} // namespace incdaw::engine::dsp
