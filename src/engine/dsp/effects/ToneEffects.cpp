#include "engine/dsp/effects/ToneEffects.h"

#include <algorithm>
#include <cmath>

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

void EqEffect::updateCoefficients() noexcept
{
    // RBJ Audio EQ Cookbook forms. Coefficients are divided through by a0.
    const auto shelf = [&](double frequency, double gainDb, bool high) {
        Coefficients out;
        const double a     = std::pow(10.0, gainDb / 40.0);
        const double omega = 2.0 * pi * std::clamp(frequency, 10.0, sampleRate_ * 0.45)
                           / sampleRate_;
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

        out.b0 = b0 / a0;
        out.b1 = b1 / a0;
        out.b2 = b2 / a0;
        out.a1 = a1 / a0;
        out.a2 = a2 / a0;
        return out;
    };

    const auto peaking = [&](double frequency, double gainDb, double qValue) {
        Coefficients out;
        const double a     = std::pow(10.0, gainDb / 40.0);
        const double omega = 2.0 * pi * std::clamp(frequency, 10.0, sampleRate_ * 0.45)
                           / sampleRate_;
        const double alpha = std::sin(omega) / (2.0 * std::max(0.1, qValue));
        const double cosw  = std::cos(omega);

        const double a0 = 1.0 + alpha / a;

        out.b0 = (1.0 + alpha * a) / a0;
        out.b1 = (-2.0 * cosw) / a0;
        out.b2 = (1.0 - alpha * a) / a0;
        out.a1 = (-2.0 * cosw) / a0;
        out.a2 = (1.0 - alpha / a) / a0;
        return out;
    };

    coefficients_[0] = shelf(cached_[0], cached_[1], false);
    coefficients_[1] = peaking(cached_[2], cached_[3], cached_[4]);
    coefficients_[2] = shelf(cached_[5], cached_[6], true);
}

void EqEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    bool dirty = false;
    for (std::size_t param = 0; param < 7; ++param) {
        const double current = valueAt(param);
        dirty          = dirty || current != cached_[param];
        cached_[param] = current;
    }

    if (dirty)
        updateCoefficients();

    // A band at exactly 0 dB is coefficient-identity; skip it entirely so
    // the defaulted EQ is bit-exact, not merely close.
    const std::size_t channels =
        std::min<std::size_t>(context.output.channelCount(), maxChannels);

    for (std::size_t band = 0; band < bandCount; ++band) {
        const double gainDb = band == 0 ? cached_[1] : band == 1 ? cached_[3] : cached_[6];
        if (gainDb == 0.0)
            continue;

        const Coefficients& c = coefficients_[band];

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

} // namespace incdaw::engine::dsp
