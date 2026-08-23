#include "engine/dsp/effects/MultibandEffects.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <iterator>
#include <numbers>

namespace incdaw::engine::dsp {
namespace {

constexpr double pi = std::numbers::pi;

/// Butterworth Q. Two of these in series make one Linkwitz-Riley fourth-order
/// half, and an LR4 lowpass and highpass sum to an allpass — which is the
/// only reason a three-band split can be flat.
constexpr double butterworthQ = 0.70710678118654752;

[[nodiscard]] BiquadCoefficients butterworthLowpass(double frequency, SampleRate rate) noexcept
{
    const double w0    = 2.0 * pi * std::clamp(frequency, 10.0, rate * 0.45) / rate;
    const double cosw  = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * butterworthQ);

    const double a0 = 1.0 + alpha;

    BiquadCoefficients c;
    c.b0 = ((1.0 - cosw) * 0.5) / a0;
    c.b1 = (1.0 - cosw) / a0;
    c.b2 = c.b0;
    c.a1 = (-2.0 * cosw) / a0;
    c.a2 = (1.0 - alpha) / a0;
    return c;
}

[[nodiscard]] BiquadCoefficients butterworthHighpass(double frequency, SampleRate rate) noexcept
{
    const double w0    = 2.0 * pi * std::clamp(frequency, 10.0, rate * 0.45) / rate;
    const double cosw  = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * butterworthQ);

    const double a0 = 1.0 + alpha;

    BiquadCoefficients c;
    c.b0 = ((1.0 + cosw) * 0.5) / a0;
    c.b1 = -(1.0 + cosw) / a0;
    c.b2 = c.b0;
    c.a1 = (-2.0 * cosw) / a0;
    c.a2 = (1.0 - alpha) / a0;
    return c;
}

/// One-pole smoothing coefficient for a time constant in milliseconds. The
/// same formula the single-band compressor uses; it is four lines, and two
/// spellings of it would be two things to keep in step.
[[nodiscard]] double coefficientFor(double milliseconds, double sampleRate) noexcept
{
    if (milliseconds <= 0.0)
        return 0.0;

    return std::exp(-1.0 / (milliseconds * 0.001 * sampleRate));
}

/// A biquad's complex response at `frequency`.
[[nodiscard]] std::complex<double> responseOf(const BiquadCoefficients& c, double frequency,
                                              SampleRate rate) noexcept
{
    const std::complex<double> z = std::polar(1.0, -2.0 * pi * frequency / rate);
    const std::complex<double> z2 = z * z;

    return (c.b0 + c.b1 * z + c.b2 * z2) / (1.0 + c.a1 * z + c.a2 * z2);
}

// ── The parameter table ──────────────────────────────────────────────────────
//
// Index order IS storage order: [0..2] the network, then seven per band. The
// audio thread reads by index; nothing scans.

using Multiband = MultibandCompressorEffect;

constexpr EffectParameter descriptors[] = {
    {Multiband::crossoverLowHz,  "Low X",   40.0,  1000.0,   200.0, false},
    {Multiband::crossoverHighHz, "High X", 500.0, 12000.0,  2500.0, false},
    {Multiband::outputDb,        "Output", -24.0,    24.0,     0.0, false},

    {Multiband::bandParameter(0, Multiband::bandThresholdDb), "Low Thresh", -60.0,    0.0,   0.0, false},
    {Multiband::bandParameter(0, Multiband::bandRatio),       "Low Ratio",    1.0,   20.0,   1.0, false},
    {Multiband::bandParameter(0, Multiband::bandAttackMs),    "Low Attack",   0.1,  200.0,  20.0, false},
    {Multiband::bandParameter(0, Multiband::bandReleaseMs),   "Low Release",  1.0, 2000.0, 200.0, false},
    {Multiband::bandParameter(0, Multiband::bandMakeupDb),    "Low Makeup",   0.0,   24.0,   0.0, false},
    {Multiband::bandParameter(0, Multiband::bandBypass),      "Low Bypass",   0.0,    1.0,   0.0, true},
    {Multiband::bandParameter(0, Multiband::bandSolo),        "Low Solo",     0.0,    1.0,   0.0, true},

    {Multiband::bandParameter(1, Multiband::bandThresholdDb), "Mid Thresh", -60.0,    0.0,   0.0, false},
    {Multiband::bandParameter(1, Multiband::bandRatio),       "Mid Ratio",    1.0,   20.0,   1.0, false},
    {Multiband::bandParameter(1, Multiband::bandAttackMs),    "Mid Attack",   0.1,  200.0,  10.0, false},
    {Multiband::bandParameter(1, Multiband::bandReleaseMs),   "Mid Release",  1.0, 2000.0, 120.0, false},
    {Multiband::bandParameter(1, Multiband::bandMakeupDb),    "Mid Makeup",   0.0,   24.0,   0.0, false},
    {Multiband::bandParameter(1, Multiband::bandBypass),      "Mid Bypass",   0.0,    1.0,   0.0, true},
    {Multiband::bandParameter(1, Multiband::bandSolo),        "Mid Solo",     0.0,    1.0,   0.0, true},

    {Multiband::bandParameter(2, Multiband::bandThresholdDb), "High Thresh", -60.0,    0.0,  0.0, false},
    {Multiband::bandParameter(2, Multiband::bandRatio),       "High Ratio",    1.0,  20.0,  1.0, false},
    {Multiband::bandParameter(2, Multiband::bandAttackMs),    "High Attack",   0.1, 200.0,  5.0, false},
    {Multiband::bandParameter(2, Multiband::bandReleaseMs),   "High Release",  1.0, 2000.0, 80.0, false},
    {Multiband::bandParameter(2, Multiband::bandMakeupDb),    "High Makeup",   0.0,  24.0,  0.0, false},
    {Multiband::bandParameter(2, Multiband::bandBypass),      "High Bypass",   0.0,   1.0,  0.0, true},
    {Multiband::bandParameter(2, Multiband::bandSolo),        "High Solo",     0.0,   1.0,  0.0, true},
};

constexpr std::size_t descriptorCount = std::size(descriptors);
constexpr std::size_t networkCount   = 3;
constexpr std::size_t perBandCount   = 7;

static_assert(descriptorCount == networkCount
                  + perBandCount * MultibandCompressorEffect::bandCount);

/// Table index of one band's control — the index form, because the audio
/// thread reads values by index and never by id.
[[nodiscard]] constexpr std::size_t bandIndex(std::size_t band,
                                              Multiband::BandOffset offset) noexcept
{
    return networkCount + band * perBandCount + static_cast<std::size_t>(offset);
}

} // namespace

MultibandCompressorEffect::MultibandCompressorEffect()
    : BuiltinEffect(descriptors, descriptorCount)
{
    envelope_.fill(1.0);
}

void MultibandCompressorEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    for (ChannelState& channel : channels_)
        channel.reset();

    envelope_.fill(1.0);
    cachedLowHz_  = -1.0;
    cachedHighHz_ = -1.0;
}

void MultibandCompressorEffect::designCrossovers(double lowHz, double highHz) noexcept
{
    if (lowHz == cachedLowHz_ && highHz == cachedHighHz_)
        return;

    lowpassLow_   = butterworthLowpass(lowHz, sampleRate_);
    highpassLow_  = butterworthHighpass(lowHz, sampleRate_);
    lowpassHigh_  = butterworthLowpass(highHz, sampleRate_);
    highpassHigh_ = butterworthHighpass(highHz, sampleRate_);

    cachedLowHz_  = lowHz;
    cachedHighHz_ = highHz;
}

void MultibandCompressorEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double outputGain = dbToGain(valueAt(2));

    // Transparency is structural. The split's sum is an allpass — flat, but
    // phase-shifted — so a multiband that is doing nothing must not run it at
    // all if it is to pass the signal through untouched. Ratio 1 with no
    // makeup and nothing soloed is exactly that case.
    bool transparent = outputGain == 1.0;
    bool anySolo     = false;

    for (std::size_t band = 0; band < bandCount && transparent; ++band) {
        transparent = transparent && valueAt(bandIndex(band, bandRatio)) == 1.0
                   && valueAt(bandIndex(band, bandMakeupDb)) == 0.0;
    }

    for (std::size_t band = 0; band < bandCount; ++band)
        anySolo = anySolo || valueAt(bandIndex(band, bandSolo)) >= 0.5;

    if (transparent && !anySolo)
        return;

    const double lowHz  = valueAt(0);
    const double highHz = std::max(valueAt(1), lowHz * 1.05);
    designCrossovers(lowHz, highHz);

    struct BandSettings {
        double threshold = 0.0;
        double slope     = 0.0;
        double attack    = 0.0;
        double release   = 0.0;
        double makeup    = 1.0;
        bool   bypassed  = false;
        bool   included  = true;
    };

    std::array<BandSettings, bandCount> bands{};

    for (std::size_t band = 0; band < bandCount; ++band) {
        const double ratio = std::max(1.0, valueAt(bandIndex(band, bandRatio)));

        bands[band].threshold = valueAt(bandIndex(band, bandThresholdDb));
        bands[band].slope     = 1.0 / ratio - 1.0;
        bands[band].attack    = coefficientFor(valueAt(bandIndex(band, bandAttackMs)), sampleRate_);
        bands[band].release   = coefficientFor(valueAt(bandIndex(band, bandReleaseMs)), sampleRate_);
        bands[band].makeup    = dbToGain(valueAt(bandIndex(band, bandMakeupDb)));
        bands[band].bypassed  = valueAt(bandIndex(band, bandBypass)) >= 0.5;
        bands[band].included  = anySolo ? valueAt(bandIndex(band, bandSolo)) >= 0.5 : true;
    }

    const std::size_t channels = std::min(context.output.channelCount(), maxChannels);

    std::array<double, bandCount> worstReduction{};

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        double split[bandCount][maxChannels] = {};
        std::array<double, bandCount> peak{};

        for (std::size_t channel = 0; channel < channels; ++channel) {
            ChannelState& state = channels_[channel];

            const double input =
                static_cast<double>(context.output.channel(channel)[frame]);

            const double lowRaw  = state.lowSplit.step(lowpassLow_, input);
            const double highRaw = state.highSplit.step(highpassLow_, input);

            // The low band takes the second crossover as an ALLPASS — its
            // lowpass and highpass halves summed — so that all three bands
            // carry the same phase and the sum stays flat.
            const double low = state.lowAllpassLow.step(lowpassHigh_, lowRaw)
                             + state.lowAllpassHigh.step(highpassHigh_, lowRaw);

            const double mid  = state.midSplit.step(lowpassHigh_, highRaw);
            const double high = state.topSplit.step(highpassHigh_, highRaw);

            split[0][channel] = low;
            split[1][channel] = mid;
            split[2][channel] = high;

            peak[0] = std::max(peak[0], std::fabs(low));
            peak[1] = std::max(peak[1], std::fabs(mid));
            peak[2] = std::max(peak[2], std::fabs(high));
        }

        std::array<double, bandCount> gain{};

        for (std::size_t band = 0; band < bandCount; ++band) {
            if (bands[band].bypassed) {
                // A bypassed band is not compressed, and its detector is left
                // where it was rather than tracking a signal it is not acting
                // on — so switching bypass off does not dump a stale gain.
                gain[band] = 1.0;
                continue;
            }

            const double level = peak[band];
            const double levelDb = level > 1.0e-10 ? 20.0 * std::log10(level) : -200.0;

            const double overDb      = levelDb - bands[band].threshold;
            const double reductionDb = overDb > 0.0 ? overDb * bands[band].slope : 0.0;
            const double target      = dbToGain(reductionDb);

            const double coefficient =
                target < envelope_[band] ? bands[band].attack : bands[band].release;
            envelope_[band] = target + coefficient * (envelope_[band] - target);

            worstReduction[band] =
                std::min(worstReduction[band],
                         20.0 * std::log10(std::max(1.0e-10, envelope_[band])));

            gain[band] = envelope_[band] * bands[band].makeup;
        }

        for (std::size_t channel = 0; channel < channels; ++channel) {
            double sum = 0.0;
            for (std::size_t band = 0; band < bandCount; ++band)
                if (bands[band].included)
                    sum += split[band][channel] * gain[band];

            context.output.channel(channel)[frame] =
                static_cast<Sample>(sum * outputGain);
        }
    }

    for (std::size_t band = 0; band < bandCount; ++band)
        reduction_[band].store(-worstReduction[band], std::memory_order_relaxed);
}

double multibandSumMagnitudeDb(double lowHz, double highHz, SampleRate sampleRate,
                               double frequency) noexcept
{
    const double top = std::max(highHz, lowHz * 1.05);

    const BiquadCoefficients lp1 = butterworthLowpass(lowHz, sampleRate);
    const BiquadCoefficients hp1 = butterworthHighpass(lowHz, sampleRate);
    const BiquadCoefficients lp2 = butterworthLowpass(top, sampleRate);
    const BiquadCoefficients hp2 = butterworthHighpass(top, sampleRate);

    // Each half is a Linkwitz-Riley fourth order: the Butterworth section
    // squared.
    const std::complex<double> LP1 = std::pow(responseOf(lp1, frequency, sampleRate), 2);
    const std::complex<double> HP1 = std::pow(responseOf(hp1, frequency, sampleRate), 2);
    const std::complex<double> LP2 = std::pow(responseOf(lp2, frequency, sampleRate), 2);
    const std::complex<double> HP2 = std::pow(responseOf(hp2, frequency, sampleRate), 2);

    const std::complex<double> low  = LP1 * (LP2 + HP2);
    const std::complex<double> mid  = HP1 * LP2;
    const std::complex<double> high = HP1 * HP2;

    const double magnitude = std::abs(low + mid + high);
    return 20.0 * std::log10(std::max(magnitude, 1.0e-12));
}

} // namespace incdaw::engine::dsp
