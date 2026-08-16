#include "engine/dsp/effects/DynamicsEffects.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine::dsp {

namespace {

constexpr EffectParameter compressorParameters[] = {
    {CompressorEffect::thresholdDb, "Threshold", -60.0,    0.0,   0.0, false},
    {CompressorEffect::ratio,       "Ratio",       1.0,   20.0,   2.0, false},
    {CompressorEffect::attackMs,    "Attack",      0.1,  200.0,  10.0, false},
    {CompressorEffect::releaseMs,   "Release",     1.0, 2000.0, 100.0, false},
    {CompressorEffect::makeupDb,    "Makeup",      0.0,   24.0,   0.0, false},
};

constexpr EffectParameter limiterParameters[] = {
    {LimiterEffect::ceilingDb, "Ceiling", -24.0,    0.0,  0.0, false},
    {LimiterEffect::releaseMs, "Release",   1.0, 1000.0, 50.0, false},
};

constexpr EffectParameter gateParameters[] = {
    {GateEffect::thresholdDb, "Threshold", -80.0,    0.0, -80.0, false},
    {GateEffect::attackMs,    "Attack",      0.1,  100.0,   1.0, false},
    {GateEffect::holdMs,      "Hold",        0.0, 1000.0,  10.0, false},
    {GateEffect::releaseMs,   "Release",     1.0, 2000.0, 100.0, false},
};

/// One-pole smoothing coefficient for a time constant in milliseconds.
double coefficientFor(double milliseconds, double sampleRate) noexcept
{
    if (milliseconds <= 0.0)
        return 0.0;

    return std::exp(-1.0 / (milliseconds * 0.001 * sampleRate));
}

/// The loudest instantaneous absolute value across the linked channels.
double linkedPeakAt(const ProcessContext& context, FrameCount frame,
                    std::size_t channels) noexcept
{
    double peak = 0.0;
    for (std::size_t channel = 0; channel < channels; ++channel)
        peak = std::max(peak,
                        std::fabs(static_cast<double>(context.output.channel(channel)[frame])));

    return peak;
}

/// The same linked detector, reading an arbitrary view — the external key.
double linkedPeakOf(const AudioBufferView& view, FrameCount frame) noexcept
{
    double peak = 0.0;
    for (std::size_t channel = 0; channel < view.channelCount(); ++channel)
        peak = std::max(peak, std::fabs(static_cast<double>(view.channel(channel)[frame])));

    return peak;
}

} // namespace

// ── CompressorEffect ─────────────────────────────────────────────────────────

CompressorEffect::CompressorEffect() : BuiltinEffect(compressorParameters, 5) {}

void CompressorEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate;
    envelope_   = 1.0;
}

void CompressorEffect::process(const ProcessContext& context) noexcept
{
    // With an external key wired in, that input feeds the detector only — it
    // must never reach the audio path.
    const bool keyed = keyInput_ != noKeyInput && keyInput_ < context.inputCount
                    && context.input(keyInput_).channelCount() > 0;

    if (keyed)
        sumInputsInto(context, keyInput_);
    else
        sumInputsInto(context);

    const double threshold = valueAt(0);
    const double ratioVal  = std::max(1.0, valueAt(1));
    const double attack    = coefficientFor(valueAt(2), sampleRate_);
    const double release   = coefficientFor(valueAt(3), sampleRate_);
    const double makeup    = dbToGain(valueAt(4));

    // Ratio 1 computes zero reduction everywhere; with no makeup the
    // multiply is by exactly 1.0. Short-circuiting keeps it bit-exact.
    if (ratioVal == 1.0 && makeup == 1.0)
        return;

    const std::size_t channels = context.output.channelCount();
    const double      slope    = 1.0 / ratioVal - 1.0;
    const AudioBufferView key  = keyed ? context.input(keyInput_) : AudioBufferView{};

    double worstReduction = 0.0;

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        const double peak   = keyed ? linkedPeakOf(key, frame) : linkedPeakAt(context, frame, channels);
        const double peakDb = peak > 1.0e-10 ? 20.0 * std::log10(peak) : -200.0;

        const double overDb      = peakDb - threshold;
        const double reductionDb = overDb > 0.0 ? overDb * slope : 0.0;
        const double target      = dbToGain(reductionDb);

        // Attack when clamping down, release when letting go.
        const double coefficient = target < envelope_ ? attack : release;
        envelope_ = target + coefficient * (envelope_ - target);

        worstReduction = std::min(worstReduction, 20.0 * std::log10(std::max(1.0e-10, envelope_)));

        const double gain = envelope_ * makeup;
        for (std::size_t channel = 0; channel < channels; ++channel) {
            Sample* samples = context.output.channel(channel);
            samples[frame]  = static_cast<Sample>(static_cast<double>(samples[frame]) * gain);
        }
    }

    reduction_.store(-worstReduction, std::memory_order_relaxed);
}

// ── LimiterEffect ────────────────────────────────────────────────────────────

LimiterEffect::LimiterEffect() : BuiltinEffect(limiterParameters, 2) {}

void LimiterEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate;
    gain_       = 1.0;
}

void LimiterEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double ceiling = dbToGain(valueAt(0));
    const double release = coefficientFor(valueAt(1), sampleRate_);

    const std::size_t channels = context.output.channelCount();

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        const double peak = linkedPeakAt(context, frame, channels);

        // Recover FIRST, clamp after: the sample is scaled by the gain that
        // exists once both have happened, so recovering after the ceiling
        // check would let a recovering gain push this very sample over it.
        gain_ = 1.0 + release * (gain_ - 1.0);

        // Instant attack: whatever gain keeps THIS sample under the ceiling.
        if (peak * gain_ > ceiling && peak > 0.0)
            gain_ = ceiling / peak;

        if (gain_ < 1.0)
            for (std::size_t channel = 0; channel < channels; ++channel) {
                Sample* samples = context.output.channel(channel);
                samples[frame] =
                    static_cast<Sample>(static_cast<double>(samples[frame]) * gain_);
            }
    }
}

// ── GateEffect ───────────────────────────────────────────────────────────────

GateEffect::GateEffect() : BuiltinEffect(gateParameters, 4) {}

void GateEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate;
    gain_       = 1.0;
    holdLeft_   = 0.0;
}

void GateEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double thresholdDbValue = valueAt(0);

    // The floor means "off": the gate is open no matter what, bit-exact.
    if (thresholdDbValue <= -80.0)
        return;

    const double threshold  = dbToGain(thresholdDbValue);
    const double attack     = coefficientFor(valueAt(1), sampleRate_);
    const double holdFrames = valueAt(2) * 0.001 * sampleRate_;
    const double release    = coefficientFor(valueAt(3), sampleRate_);

    const std::size_t channels = context.output.channelCount();

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        const double peak = linkedPeakAt(context, frame, channels);

        double target;
        if (peak >= threshold) {
            target    = 1.0;
            holdLeft_ = holdFrames;
        } else if (holdLeft_ > 0.0) {
            target = 1.0;
            holdLeft_ -= 1.0;
        } else {
            target = 0.0;
        }

        const double coefficient = target > gain_ ? attack : release;
        gain_ = target + coefficient * (gain_ - target);

        for (std::size_t channel = 0; channel < channels; ++channel) {
            Sample* samples = context.output.channel(channel);
            samples[frame]  = static_cast<Sample>(static_cast<double>(samples[frame]) * gain_);
        }
    }
}

} // namespace incdaw::engine::dsp
