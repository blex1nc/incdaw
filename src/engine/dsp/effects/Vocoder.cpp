#include "engine/dsp/effects/Vocoder.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>

namespace incdaw::engine::dsp {
namespace {

using Voc = VocoderEffect;

constexpr double pi = std::numbers::pi;

constexpr EffectParameter descriptors[] = {
    {Voc::mix,       "Mix",         0.0,   1.0,   0.0, false},
    {Voc::bandCount, "Bands",       4.0,  32.0,  20.0, true},
    {Voc::attackMs,  "Attack",      0.1, 200.0,   4.0, false},
    {Voc::releaseMs, "Release",     1.0, 500.0,  35.0, false},
    {Voc::formant,   "Formant",   -12.0,  12.0,   0.0, false},
    {Voc::resonance, "Resonance",   1.0,  12.0,   4.0, false},
    {Voc::sibilance, "Sibilance",   0.0,   1.0,   0.3, false},
    {Voc::outputDb,  "Output",    -24.0,  24.0,   0.0, false},
};

constexpr std::size_t descriptorCount = std::size(descriptors);

/// One-pole smoothing coefficient for a time constant in milliseconds.
[[nodiscard]] double coefficientFor(double milliseconds, double sampleRate) noexcept
{
    if (milliseconds <= 0.0)
        return 0.0;

    return std::exp(-1.0 / (milliseconds * 0.001 * sampleRate));
}

} // namespace

double VocoderEffect::bandCentreHz(std::size_t index, std::size_t count) noexcept
{
    if (count < 2)
        return std::sqrt(lowestBandHz * highestBandHz);

    // Logarithmic spacing: the ear hears ratios, and equal spacing in hertz
    // would put twenty of the bands above the top of a voice.
    const double position = static_cast<double>(index) / static_cast<double>(count - 1);
    return lowestBandHz * std::pow(highestBandHz / lowestBandHz, position);
}

VocoderEffect::VocoderEffect() : BuiltinEffect(descriptors, descriptorCount) {}

void VocoderEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    for (auto& channel : carrierBands_)
        for (Band& band : channel)
            band.reset();

    for (auto& channel : modulatorBands_)
        for (Band& band : channel)
            band.reset();

    envelope_.fill(0.0);

    for (LinkwitzRileyHalf& half : sibilanceFilter_)
        half.reset();

    sibilanceCoefficients_ = butterworthHighpass(highestBandHz, sampleRate_);
}

void VocoderEffect::process(const ProcessContext& context) noexcept
{
    const bool keyed = keyInput_ != KeyedEffect::noKeyInput
                    && keyInput_ < context.inputCount
                    && context.input(keyInput_).channelCount() > 0;

    // The modulator must never reach the audio path — it is a control signal,
    // not something to be heard.
    if (keyed)
        sumInputsInto(context, keyInput_);
    else
        sumInputsInto(context);

    const double wet = valueAt(0);

    // Mix at zero, or no modulator wired in, and the carrier passes through
    // untouched. A vocoder with nothing to say must not colour the sound it
    // was going to speak through.
    if (wet == 0.0 || !keyed)
        return;

    const auto   bands      = static_cast<std::size_t>(
        std::clamp(valueAt(1), 4.0, static_cast<double>(maxBands)));
    const double attack     = coefficientFor(valueAt(2), sampleRate_);
    const double release    = coefficientFor(valueAt(3), sampleRate_);
    const double formantRatio = std::exp2(valueAt(4) / 12.0);
    const double damping    = 1.0 / std::max(1.0, valueAt(5));
    const double sibilanceAmount = valueAt(6);
    const double outputGain = dbToGain(valueAt(7)) * wet;
    const double dry        = 1.0 - wet;

    const AudioBufferView key = context.input(keyInput_);

    const std::size_t channels = std::min(context.output.channelCount(), maxChannels);
    const std::size_t keyChannels = key.channelCount();

    // Band coefficients, once per block: they depend on the parameters, not
    // on the sample.
    double carrierF[maxBands]{};
    double modulatorF[maxBands]{};

    const double nyquist = sampleRate_ * 0.49;

    for (std::size_t band = 0; band < bands; ++band) {
        const double centre = bandCentreHz(band, bands);

        modulatorF[band] =
            2.0 * std::sin(pi * std::min(centre, nyquist) / sampleRate_);
        carrierF[band] =
            2.0 * std::sin(pi * std::min(centre * formantRatio, nyquist) / sampleRate_);
    }

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        // ── The modulator, mono: what is being said is one thing, however
        //    many channels it arrived on.
        double modulator = 0.0;
        for (std::size_t channel = 0; channel < keyChannels; ++channel)
            modulator += static_cast<double>(key.channel(channel)[frame]);

        if (keyChannels > 1)
            modulator /= static_cast<double>(keyChannels);

        for (std::size_t band = 0; band < bands; ++band) {
            const double analysed =
                modulatorBands_[0][band].step(modulator, modulatorF[band], damping);

            const double rectified = std::fabs(analysed);
            const double coefficient = rectified > envelope_[band] ? attack : release;

            envelope_[band] = rectified + coefficient * (envelope_[band] - rectified);
        }

        // Consonants have no pitch for the bands to grab, so the modulator's
        // top end goes straight across. Without this a vocoder cannot say an
        // "s" at all.
        const double sibilant =
            sibilanceAmount > 0.0
                ? sibilanceFilter_[0].step(sibilanceCoefficients_, modulator) * sibilanceAmount
                : 0.0;

        for (std::size_t channel = 0; channel < channels; ++channel) {
            const double carrier = static_cast<double>(context.output.channel(channel)[frame]);

            double voiced = 0.0;
            for (std::size_t band = 0; band < bands; ++band)
                voiced += carrierBands_[channel][band].step(carrier, carrierF[band], damping)
                        * envelope_[band];

            // The bank's bands overlap, so their sum runs hot; the envelope
            // is a magnitude rather than a gain, so it needs the same
            // correction. Two is what brings a full-scale carrier back to
            // roughly full scale.
            voiced *= 2.0;

            context.output.channel(channel)[frame] =
                static_cast<Sample>(carrier * dry + (voiced + sibilant) * outputGain);
        }
    }
}

} // namespace incdaw::engine::dsp
