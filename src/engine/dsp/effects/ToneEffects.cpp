#include "engine/dsp/effects/ToneEffects.h"
#include "engine/dsp/effects/EffectRegistry.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace incdaw::engine::dsp {

namespace {

constexpr double pi = 3.14159265358979323846;

constexpr EffectParameter filterParameters[] = {
    {FilterEffect::mode,      "Mode",       0.0,     3.0,     0.0,    true},
    {FilterEffect::cutoffHz,  "Cutoff",    20.0, 20000.0, 20000.0,   false},
    {FilterEffect::resonance, "Resonance",  0.1,    10.0,     0.7071, false},
};

constexpr EffectParameter eqParameters[] = {
    {EqEffect::lowFreq,    "Low Freq",     20.0,  2000.0,  120.0, false},
    {EqEffect::lowGainDb,  "Low Gain",    -24.0,    24.0,    0.0, false},
    {EqEffect::midFreq,    "Mid Freq",    100.0, 10000.0, 1000.0, false},
    {EqEffect::midGainDb,  "Mid Gain",    -24.0,    24.0,    0.0, false},
    {EqEffect::midQ,       "Mid Q",         0.1,    10.0,    1.0, false},
    {EqEffect::highFreq,   "High Freq",  1000.0, 20000.0, 8000.0, false},
    {EqEffect::highGainDb, "High Gain",   -24.0,    24.0,    0.0, false},
};

constexpr EffectParameter saturatorParameters[] = {
    {SaturatorEffect::driveDb, "Drive", 0.0, 36.0, 0.0, false},
    {SaturatorEffect::mix,     "Mix",   0.0,  1.0, 1.0, false},
};

} // namespace

// ── FilterEffect ─────────────────────────────────────────────────────────────

FilterEffect::FilterEffect() : BuiltinEffect(filterParameters, 3) {}

void FilterEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate;
    for (State& state : states_)
        state = {};
}

void FilterEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const int filterMode = static_cast<int>(valueAt(0));
    if (filterMode == off)
        return;

    const double cutoff = std::clamp(valueAt(1), 20.0, sampleRate_ * 0.24);
    const double q      = 1.0 / std::max(0.1, valueAt(2));
    const double f      = 2.0 * std::sin(pi * cutoff / sampleRate_);

    const std::size_t channels =
        std::min<std::size_t>(context.output.channelCount(), maxChannels);

    for (std::size_t channel = 0; channel < channels; ++channel) {
        Sample* samples = context.output.channel(channel);
        State&  state   = states_[channel];

        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            const double input = static_cast<double>(samples[frame]);

            state.low += f * state.band;
            const double high = input - state.low - q * state.band;
            state.band += f * high;

            const double value = filterMode == lowpass    ? state.low
                                 : filterMode == highpass ? high
                                                          : state.band;
            samples[frame] = static_cast<Sample>(value);
        }
    }
}

// ── EqEffect ─────────────────────────────────────────────────────────────────

EqEffect::EqEffect() : BuiltinEffect(eqParameters, 7) {}

void EqEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate;
    for (auto& band : states_)
        for (State& state : band)
            state = {};
    cached_[0] = -1.0;   // force a coefficient rebuild
}

void designEqBands(const double parameters[EqEffect::paramCount], SampleRate sampleRate,
                   BiquadCoefficients out[EqEffect::bandCount]) noexcept
{
    // RBJ Audio EQ Cookbook forms. Coefficients are divided through by a0.
    const auto shelf = [&](double frequency, double gainDb, bool high) {
        BiquadCoefficients result;
        const double a     = std::pow(10.0, gainDb / 40.0);
        const double omega = 2.0 * pi * std::clamp(frequency, 10.0, sampleRate * 0.45)
                           / sampleRate;
        const double cosw  = std::cos(omega);
        const double sinw  = std::sin(omega);
        const double slope = 1.0;
        const double alpha = sinw / 2.0
                           * std::sqrt((a + 1.0 / a) * (1.0 / slope - 1.0) + 2.0);
        const double twoRootAAlpha = 2.0 * std::sqrt(a) * alpha;

        const double sign = high ? 1.0 : -1.0;

        const double b0 = a * ((a + 1.0) + sign * (a - 1.0) * cosw + twoRootAAlpha);
        const double b1 = -2.0 * sign * a * ((a - 1.0) + sign * (a + 1.0) * cosw);
        const double b2 = a * ((a + 1.0) + sign * (a - 1.0) * cosw - twoRootAAlpha);
        const double a0 = (a + 1.0) - sign * (a - 1.0) * cosw + twoRootAAlpha;
        const double a1 = 2.0 * sign * ((a - 1.0) - sign * (a + 1.0) * cosw);
        const double a2 = (a + 1.0) - sign * (a - 1.0) * cosw - twoRootAAlpha;

        result.b0 = b0 / a0;
        result.b1 = b1 / a0;
        result.b2 = b2 / a0;
        result.a1 = a1 / a0;
        result.a2 = a2 / a0;
        return result;
    };

    const auto peaking = [&](double frequency, double gainDb, double qValue) {
        BiquadCoefficients result;
        const double a     = std::pow(10.0, gainDb / 40.0);
        const double omega = 2.0 * pi * std::clamp(frequency, 10.0, sampleRate * 0.45)
                           / sampleRate;
        const double alpha = std::sin(omega) / (2.0 * std::max(0.1, qValue));
        const double cosw  = std::cos(omega);

        const double a0 = 1.0 + alpha / a;

        result.b0 = (1.0 + alpha * a) / a0;
        result.b1 = (-2.0 * cosw) / a0;
        result.b2 = (1.0 - alpha * a) / a0;
        result.a1 = (-2.0 * cosw) / a0;
        result.a2 = (1.0 - alpha / a) / a0;
        return result;
    };

    out[0] = shelf(parameters[EqEffect::lowFreq], parameters[EqEffect::lowGainDb], false);
    out[1] = peaking(parameters[EqEffect::midFreq], parameters[EqEffect::midGainDb],
                     parameters[EqEffect::midQ]);
    out[2] = shelf(parameters[EqEffect::highFreq], parameters[EqEffect::highGainDb], true);
}

double eqMagnitudeDb(const double parameters[EqEffect::paramCount], SampleRate sampleRate,
                     double frequency) noexcept
{
    BiquadCoefficients bands[EqEffect::bandCount];
    designEqBands(parameters, sampleRate, bands);

    const double omega = 2.0 * pi * std::clamp(frequency, 1.0, sampleRate * 0.49) / sampleRate;
    const double cos1 = std::cos(omega),      sin1 = std::sin(omega);
    const double cos2 = std::cos(2.0 * omega), sin2 = std::sin(2.0 * omega);

    double total = 0.0;

    for (std::size_t band = 0; band < EqEffect::bandCount; ++band) {
        // The same skip `process` makes: a band at exactly 0 dB is identity.
        const double gainDb = band == 0   ? parameters[EqEffect::lowGainDb]
                              : band == 1 ? parameters[EqEffect::midGainDb]
                                          : parameters[EqEffect::highGainDb];
        if (gainDb == 0.0)
            continue;

        const BiquadCoefficients& c = bands[band];

        const double numeratorReal   = c.b0 + c.b1 * cos1 + c.b2 * cos2;
        const double numeratorImag   = -(c.b1 * sin1 + c.b2 * sin2);
        const double denominatorReal = 1.0 + c.a1 * cos1 + c.a2 * cos2;
        const double denominatorImag = -(c.a1 * sin1 + c.a2 * sin2);

        const double numerator   = numeratorReal * numeratorReal
                                 + numeratorImag * numeratorImag;
        const double denominator = denominatorReal * denominatorReal
                                 + denominatorImag * denominatorImag;

        if (denominator <= 0.0 || numerator <= 0.0)
            continue;

        total += 10.0 * std::log10(numerator / denominator);
    }

    return total;
}

void EqEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    bool dirty = false;
    for (std::size_t param = 0; param < paramCount; ++param) {
        const double current = valueAt(param);
        dirty          = dirty || current != cached_[param];
        cached_[param] = current;
    }

    if (dirty)
        designEqBands(cached_, sampleRate_, coefficients_);

    // A band at exactly 0 dB is coefficient-identity; skip it entirely so
    // the defaulted EQ is bit-exact, not merely close.
    const std::size_t channels =
        std::min<std::size_t>(context.output.channelCount(), maxChannels);

    for (std::size_t band = 0; band < bandCount; ++band) {
        const double gainDb = band == 0 ? cached_[1] : band == 1 ? cached_[3] : cached_[6];
        if (gainDb == 0.0)
            continue;

        const BiquadCoefficients& c = coefficients_[band];

        for (std::size_t channel = 0; channel < channels; ++channel) {
            Sample* samples = context.output.channel(channel);
            State&  state   = states_[band][channel];

            for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
                const double input  = static_cast<double>(samples[frame]);
                const double output = c.b0 * input + state.z1;

                state.z1 = c.b1 * input - c.a1 * output + state.z2;
                state.z2 = c.b2 * input - c.a2 * output;

                samples[frame] = static_cast<Sample>(output);
            }
        }
    }
}

// ── SaturatorEffect ──────────────────────────────────────────────────────────

SaturatorEffect::SaturatorEffect() : BuiltinEffect(saturatorParameters, 2) {}

void SaturatorEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double drive   = valueAt(0);
    const double wetness = valueAt(1);

    // Drive 0 is transparency by definition, structurally rather than by a
    // formula that merely approaches identity.
    if (drive <= 0.0 || wetness <= 0.0)
        return;

    const double gain = dbToGain(drive);

    for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel) {
        Sample* samples = context.output.channel(channel);

        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            const double dry    = static_cast<double>(samples[frame]);
            const double shaped = std::tanh(dry * gain) / gain;
            samples[frame] = static_cast<Sample>(shaped * wetness + dry * (1.0 - wetness));
        }
    }
}

// ── Registrar ────────────────────────────────────────────────────────────────

void registerToneEffects(std::vector<EffectCatalogueEntry>& rows)
{
    addEffect(rows, "incdaw.filter",    "Filter",
              [](SampleRate) { return std::make_unique<FilterEffect>(); });
    addEffect(rows, "incdaw.eq",        "EQ 3-Band",
              [](SampleRate) { return std::make_unique<EqEffect>(); });

    // The same three-band EQ under a mixing-desk face: the shell gives this
    // uid a Bass/Mid/Treble panel with a response curve instead of seven
    // sliders. One uid, one DSP class — a second tone stack would be the
    // same filter written twice (CLAUDE.md §34).
    addEffect(rows, "incdaw.tone",      "Tone (Bass/Mid/Treble)",
              [](SampleRate) { return std::make_unique<EqEffect>(); });
    addEffect(rows, "incdaw.saturator", "Saturator",
              [](SampleRate) { return std::make_unique<SaturatorEffect>(); });
}

} // namespace incdaw::engine::dsp
