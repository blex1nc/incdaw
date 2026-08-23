#pragma once

// Vocoder (A12) — the modulator's shape, on the carrier's sound.
//
// Two signals go in. The CARRIER is what the insert is on: a synth pad, a
// saw, whatever is to be spoken through. The MODULATOR arrives on a sidechain
// edge — the same wiring the compressor's key uses, resolved against
// KeyedEffect so the graph compiler needs no list of which effects take one.
//
// Both are split into the same bank of bandpass filters. Each band of the
// modulator is followed by an envelope, and that envelope becomes the gain on
// the matching band of the carrier. The sum is the carrier wearing the
// modulator's spectrum, which is what makes a synth talk.
//
// The two details that separate a vocoder from a novelty: the top end, where
// consonants live and there is nothing periodic for the bands to grab, is
// passed through from the modulator directly; and the carrier's bands can be
// shifted against the modulator's, which is what a formant control is.

#include "engine/dsp/effects/BuiltinEffect.h"
#include "engine/dsp/effects/Crossover.h"

#include <array>
#include <cstddef>

namespace incdaw::engine::dsp {

class VocoderEffect final : public BuiltinEffect, public KeyedEffect {
public:
    /// The most bands the bank will run. Above about thirty the bands are
    /// narrower than a formant and the result gets thin rather than clearer.
    static constexpr std::size_t maxBands   = 32;
    static constexpr std::size_t maxChannels = 2;

    /// The range the bank is spread across, logarithmically — speech lives
    /// inside it and the bands below 80 Hz would only pass rumble.
    static constexpr double lowestBandHz  = 90.0;
    static constexpr double highestBandHz = 9000.0;

    /// Ids are frozen: they key the state blob and every saved preset.
    enum Param : std::uint32_t {
        mix        = 0,
        bandCount  = 1,
        attackMs   = 2,
        releaseMs  = 3,
        formant    = 4,   ///< semitones the carrier's bands are shifted by
        resonance  = 5,   ///< Q of every band
        sibilance  = 6,   ///< how much of the modulator's top passes straight
        outputDb   = 7,
    };

    VocoderEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Vocoder"; }

    void setKeyInput(std::size_t index) noexcept override { keyInput_ = index; }
    [[nodiscard]] std::size_t keyInput() const noexcept override { return keyInput_; }

    /// The centre of band `index` of `count`, in Hz. Free so a view and the
    /// filter cannot disagree about where the bands are.
    [[nodiscard]] static double bandCentreHz(std::size_t index, std::size_t count) noexcept;

private:
    /// One band on one channel: a bandpass, as a state-variable filter.
    struct Band {
        double low = 0.0, band = 0.0;

        void reset() noexcept { low = 0.0; band = 0.0; }

        /// Returns the bandpass output and advances.
        [[nodiscard]] double step(double input, double f, double damping) noexcept
        {
            const double high = input - low - damping * band;
            band += f * high;
            low  += f * band;
            return band;
        }
    };

    std::array<std::array<Band, maxBands>, maxChannels> carrierBands_{};
    std::array<std::array<Band, maxBands>, maxChannels> modulatorBands_{};

    /// One envelope per band, linked across channels: a vocoder that tracked
    /// the two sides separately would wander in the stereo field on every
    /// syllable.
    std::array<double, maxBands> envelope_{};

    /// The highpass that lifts consonants out of the modulator.
    LinkwitzRileyHalf sibilanceFilter_[maxChannels];
    BiquadCoefficients sibilanceCoefficients_{};

    std::size_t keyInput_   = KeyedEffect::noKeyInput;
    SampleRate  sampleRate_ = 48000.0;
};

} // namespace incdaw::engine::dsp
