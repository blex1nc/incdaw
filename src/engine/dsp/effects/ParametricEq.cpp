#include "engine/dsp/effects/ParametricEq.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <iterator>
#include <numbers>

namespace incdaw::engine::dsp {
namespace {

using Eq = ParametricEqEffect;

constexpr double pi = std::numbers::pi;

/// The eight bands' defaults: spread across the spectrum, all switched off,
/// so a fresh EQ is transparent and every band is somewhere sensible when it
/// is switched on.
constexpr double defaultFrequency[parametricBandCount] = {
    60.0, 150.0, 350.0, 800.0, 1800.0, 4000.0, 8000.0, 14000.0,
};

constexpr EffectParameter descriptors[] = {
    {Eq::outputDb, "Output", -24.0, 24.0, 0.0, false},

    {Eq::bandParameter(0, Eq::bandType),      "B1 Type", 0.0, 6.0, 0.0, true},
    {Eq::bandParameter(0, Eq::bandFrequency), "B1 Freq", 20.0, 20000.0, defaultFrequency[0], false},
    {Eq::bandParameter(0, Eq::bandGainDb),    "B1 Gain", -24.0, 24.0, 0.0, false},
    {Eq::bandParameter(0, Eq::bandQ),         "B1 Q",    0.1, 18.0, 0.707, false},

    {Eq::bandParameter(1, Eq::bandType),      "B2 Type", 0.0, 6.0, 0.0, true},
    {Eq::bandParameter(1, Eq::bandFrequency), "B2 Freq", 20.0, 20000.0, defaultFrequency[1], false},
    {Eq::bandParameter(1, Eq::bandGainDb),    "B2 Gain", -24.0, 24.0, 0.0, false},
    {Eq::bandParameter(1, Eq::bandQ),         "B2 Q",    0.1, 18.0, 0.707, false},

    {Eq::bandParameter(2, Eq::bandType),      "B3 Type", 0.0, 6.0, 0.0, true},
    {Eq::bandParameter(2, Eq::bandFrequency), "B3 Freq", 20.0, 20000.0, defaultFrequency[2], false},
    {Eq::bandParameter(2, Eq::bandGainDb),    "B3 Gain", -24.0, 24.0, 0.0, false},
    {Eq::bandParameter(2, Eq::bandQ),         "B3 Q",    0.1, 18.0, 0.707, false},

    {Eq::bandParameter(3, Eq::bandType),      "B4 Type", 0.0, 6.0, 0.0, true},
    {Eq::bandParameter(3, Eq::bandFrequency), "B4 Freq", 20.0, 20000.0, defaultFrequency[3], false},
    {Eq::bandParameter(3, Eq::bandGainDb),    "B4 Gain", -24.0, 24.0, 0.0, false},
    {Eq::bandParameter(3, Eq::bandQ),         "B4 Q",    0.1, 18.0, 0.707, false},

    {Eq::bandParameter(4, Eq::bandType),      "B5 Type", 0.0, 6.0, 0.0, true},
    {Eq::bandParameter(4, Eq::bandFrequency), "B5 Freq", 20.0, 20000.0, defaultFrequency[4], false},
    {Eq::bandParameter(4, Eq::bandGainDb),    "B5 Gain", -24.0, 24.0, 0.0, false},
    {Eq::bandParameter(4, Eq::bandQ),         "B5 Q",    0.1, 18.0, 0.707, false},

    {Eq::bandParameter(5, Eq::bandType),      "B6 Type", 0.0, 6.0, 0.0, true},
    {Eq::bandParameter(5, Eq::bandFrequency), "B6 Freq", 20.0, 20000.0, defaultFrequency[5], false},
    {Eq::bandParameter(5, Eq::bandGainDb),    "B6 Gain", -24.0, 24.0, 0.0, false},
    {Eq::bandParameter(5, Eq::bandQ),         "B6 Q",    0.1, 18.0, 0.707, false},

    {Eq::bandParameter(6, Eq::bandType),      "B7 Type", 0.0, 6.0, 0.0, true},
    {Eq::bandParameter(6, Eq::bandFrequency), "B7 Freq", 20.0, 20000.0, defaultFrequency[6], false},
    {Eq::bandParameter(6, Eq::bandGainDb),    "B7 Gain", -24.0, 24.0, 0.0, false},
    {Eq::bandParameter(6, Eq::bandQ),         "B7 Q",    0.1, 18.0, 0.707, false},

    {Eq::bandParameter(7, Eq::bandType),      "B8 Type", 0.0, 6.0, 0.0, true},
    {Eq::bandParameter(7, Eq::bandFrequency), "B8 Freq", 20.0, 20000.0, defaultFrequency[7], false},
    {Eq::bandParameter(7, Eq::bandGainDb),    "B8 Gain", -24.0, 24.0, 0.0, false},
    {Eq::bandParameter(7, Eq::bandQ),         "B8 Q",    0.1, 18.0, 0.707, false},
};

constexpr std::size_t descriptorCount = std::size(descriptors);
static_assert(descriptorCount == 1 + parametricBandCount * 4);

/// Table index of one band's control. The audio thread reads by index.
[[nodiscard]] constexpr std::size_t bandIndex(std::size_t band, Eq::BandOffset offset) noexcept
{
    return 1 + band * 4 + static_cast<std::size_t>(offset);
}

[[nodiscard]] bool sameBand(const ParametricBand& a, const ParametricBand& b) noexcept
{
    return a.type == b.type && a.frequency == b.frequency && a.gainDb == b.gainDb
        && a.q == b.q;
}

} // namespace

const char* parametricBandTypeName(ParametricBandType type) noexcept
{
    switch (type) {
        case ParametricBandType::off:       return "Off";
        case ParametricBandType::lowShelf:  return "Low Shelf";
        case ParametricBandType::peak:      return "Peak";
        case ParametricBandType::highShelf: return "High Shelf";
        case ParametricBandType::lowPass:   return "Low Pass";
        case ParametricBandType::highPass:  return "High Pass";
        case ParametricBandType::notch:     return "Notch";
    }

    return "Off";
}

BiquadCoefficients designParametricBand(const ParametricBand& band, SampleRate rate) noexcept
{
    BiquadCoefficients identity;

    if (band.type == ParametricBandType::off)
        return identity;

    // A shelf or a peak at exactly 0 dB is the identity too. Skipping it is
    // not an optimisation: it is what makes a defaulted EQ null bit-exactly
    // rather than to within the arithmetic.
    const bool gainShaped = band.type == ParametricBandType::lowShelf
                         || band.type == ParametricBandType::peak
                         || band.type == ParametricBandType::highShelf;

    if (gainShaped && band.gainDb == 0.0)
        return identity;

    const double frequency = std::clamp(band.frequency, 10.0, rate * 0.49);
    const double q         = std::max(band.q, 0.05);

    const double w0    = 2.0 * pi * frequency / rate;
    const double cosw  = std::cos(w0);
    const double sinw  = std::sin(w0);
    const double alpha = sinw / (2.0 * q);
    const double A     = std::pow(10.0, band.gainDb / 40.0);

    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a0 = 1.0, a1 = 0.0, a2 = 0.0;

    switch (band.type) {
        case ParametricBandType::peak:
            b0 = 1.0 + alpha * A;
            b1 = -2.0 * cosw;
            b2 = 1.0 - alpha * A;
            a0 = 1.0 + alpha / A;
            a1 = -2.0 * cosw;
            a2 = 1.0 - alpha / A;
            break;

        case ParametricBandType::lowShelf: {
            const double root = 2.0 * std::sqrt(A) * alpha;
            b0 = A * ((A + 1.0) - (A - 1.0) * cosw + root);
            b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw);
            b2 = A * ((A + 1.0) - (A - 1.0) * cosw - root);
            a0 = (A + 1.0) + (A - 1.0) * cosw + root;
            a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosw);
            a2 = (A + 1.0) + (A - 1.0) * cosw - root;
            break;
        }

        case ParametricBandType::highShelf: {
            const double root = 2.0 * std::sqrt(A) * alpha;
            b0 = A * ((A + 1.0) + (A - 1.0) * cosw + root);
            b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw);
            b2 = A * ((A + 1.0) + (A - 1.0) * cosw - root);
            a0 = (A + 1.0) - (A - 1.0) * cosw + root;
            a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosw);
            a2 = (A + 1.0) - (A - 1.0) * cosw - root;
            break;
        }

        case ParametricBandType::lowPass:
            b0 = (1.0 - cosw) * 0.5;
            b1 = 1.0 - cosw;
            b2 = b0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosw;
            a2 = 1.0 - alpha;
            break;

        case ParametricBandType::highPass:
            b0 = (1.0 + cosw) * 0.5;
            b1 = -(1.0 + cosw);
            b2 = b0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosw;
            a2 = 1.0 - alpha;
            break;

        case ParametricBandType::notch:
            b0 = 1.0;
            b1 = -2.0 * cosw;
            b2 = 1.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosw;
            a2 = 1.0 - alpha;
            break;

        case ParametricBandType::off:
            return identity;
    }

    BiquadCoefficients c;
    c.b0 = b0 / a0;
    c.b1 = b1 / a0;
    c.b2 = b2 / a0;
    c.a1 = a1 / a0;
    c.a2 = a2 / a0;
    return c;
}

double parametricMagnitudeDb(const std::array<ParametricBand, parametricBandCount>& bands,
                             SampleRate sampleRate, double frequency) noexcept
{
    const std::complex<double> z  = std::polar(1.0, -2.0 * pi * frequency / sampleRate);
    const std::complex<double> z2 = z * z;

    double magnitude = 1.0;

    for (const ParametricBand& band : bands) {
        const BiquadCoefficients c = designParametricBand(band, sampleRate);

        const std::complex<double> response =
            (c.b0 + c.b1 * z + c.b2 * z2) / (1.0 + c.a1 * z + c.a2 * z2);

        magnitude *= std::abs(response);
    }

    return 20.0 * std::log10(std::max(magnitude, 1.0e-12));
}

ParametricEqEffect::ParametricEqEffect() : BuiltinEffect(descriptors, descriptorCount) {}

void ParametricEqEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    for (auto& band : states_)
        for (State& state : band)
            state = {};

    designed_ = false;
}

std::array<ParametricBand, ParametricEqEffect::bandCount> ParametricEqEffect::bands() const noexcept
{
    std::array<ParametricBand, bandCount> result{};

    for (std::size_t band = 0; band < bandCount; ++band) {
        result[band].type = static_cast<ParametricBandType>(std::clamp(
            static_cast<int>(valueAt(bandIndex(band, bandType))), 0,
            parametricBandTypeCount - 1));
        result[band].frequency = valueAt(bandIndex(band, bandFrequency));
        result[band].gainDb    = valueAt(bandIndex(band, bandGainDb));
        result[band].q         = valueAt(bandIndex(band, bandQ));
    }

    return result;
}

void ParametricEqEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const std::array<ParametricBand, bandCount> wanted = bands();
    const double outputGain = dbToGain(valueAt(0));

    for (std::size_t band = 0; band < bandCount; ++band) {
        if (designed_ && sameBand(wanted[band], cached_[band]))
            continue;

        coefficients_[band] = designParametricBand(wanted[band], sampleRate_);
        cached_[band]       = wanted[band];
    }

    designed_ = true;

    const std::size_t channels = std::min(context.output.channelCount(), maxChannels);

    for (std::size_t band = 0; band < bandCount; ++band) {
        // An identity band is skipped rather than run: a biquad that
        // multiplies by one still costs five multiplies and, worse, still
        // rounds. Skipping is what makes the defaulted EQ bit-exact.
        if (wanted[band].type == ParametricBandType::off)
            continue;

        const BiquadCoefficients& c = coefficients_[band];
        if (c.b0 == 1.0 && c.b1 == 0.0 && c.b2 == 0.0 && c.a1 == 0.0 && c.a2 == 0.0)
            continue;

        for (std::size_t channel = 0; channel < channels; ++channel) {
            State&  state   = states_[band][channel];
            Sample* samples = context.output.channel(channel);

            for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
                const double input  = static_cast<double>(samples[frame]);
                const double output = c.b0 * input + state.z1;

                state.z1 = c.b1 * input - c.a1 * output + state.z2;
                state.z2 = c.b2 * input - c.a2 * output;

                samples[frame] = static_cast<Sample>(output);
            }
        }
    }

    if (outputGain != 1.0)
        for (std::size_t channel = 0; channel < channels; ++channel) {
            Sample* samples = context.output.channel(channel);
            for (FrameCount frame = 0; frame < context.frameCount; ++frame)
                samples[frame] =
                    static_cast<Sample>(static_cast<double>(samples[frame]) * outputGain);
        }
}

} // namespace incdaw::engine::dsp
