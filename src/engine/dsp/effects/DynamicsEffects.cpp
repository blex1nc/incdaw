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

    double worstReduction = 0.0;

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        const double peak   = linkedPeakAt(context, frame, channels);
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

// ── LookaheadLimiterEffect ───────────────────────────────────────────────────

namespace {

constexpr EffectParameter lookaheadLimiterParameters[] = {
    {LookaheadLimiterEffect::ceilingDb, "Ceiling", -24.0,    0.0,  0.0, false},
    {LookaheadLimiterEffect::releaseMs, "Release",   1.0, 1000.0, 50.0, false},
};

} // namespace

LookaheadLimiterEffect::LookaheadLimiterEffect(SampleRate sampleRate)
    : BuiltinEffect(lookaheadLimiterParameters, 2), sampleRate_(sampleRate)
{
    // Latency is reported from here: the graph's topology reads it before
    // prepare runs, so it must not wait for prepare to be known.
    lookahead_ = std::max<FrameCount>(
        1, static_cast<FrameCount>(lookaheadMilliseconds * 0.001 * sampleRate));
}

void LookaheadLimiterEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    (void)sampleRate;   // the compile options' rate — the constructor's own

    gain_       = 1.0;
    writeIndex_ = 0;
    frameClock_ = 0;
    dequeHead_  = 0;
    dequeTail_  = 0;

    for (auto& channel : delay_)
        channel.assign(static_cast<std::size_t>(lookahead_), 0.0);

    const auto capacity = static_cast<std::size_t>(lookahead_) + 2;
    dequeFrames_.assign(capacity, 0);
    dequeValues_.assign(capacity, 0.0);
    windowPeaks_.assign(capacity, 0.0);
}

void LookaheadLimiterEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double ceiling = dbToGain(valueAt(0));
    const double release = coefficientFor(valueAt(1), sampleRate_);

    const std::size_t channels =
        std::min<std::size_t>(context.output.channelCount(), 2);
    const auto capacity = dequeFrames_.size();
    const auto window   = static_cast<FrameCount>(lookahead_) + 1;

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        const double peak = linkedPeakAt(context, frame, channels);

        // Sliding maximum over the trailing window: pop dominated tails,
        // expire the head that left the window, push this frame.
        while (dequeTail_ != dequeHead_
               && dequeValues_[(dequeTail_ + capacity - 1) % capacity] <= peak)
            dequeTail_ = (dequeTail_ + capacity - 1) % capacity;

        while (dequeTail_ != dequeHead_
               && dequeFrames_[dequeHead_] + window <= frameClock_)
            dequeHead_ = (dequeHead_ + 1) % capacity;

        dequeFrames_[dequeTail_] = frameClock_;
        dequeValues_[dequeTail_] = peak;
        dequeTail_               = (dequeTail_ + 1) % capacity;

        const double windowMax = dequeValues_[dequeHead_];

        // Recover first, clamp after — and the clamp sees the window's
        // maximum, so the gain lands BEFORE the transient that needs it.
        gain_ = 1.0 + release * (gain_ - 1.0);

        if (windowMax > 0.0 && windowMax * gain_ > ceiling)
            gain_ = ceiling / windowMax;

        for (std::size_t channel = 0; channel < channels; ++channel) {
            Sample* samples = context.output.channel(channel);

            const double incoming = static_cast<double>(samples[frame]);
            const double delayed  = delay_[channel][writeIndex_];

            delay_[channel][writeIndex_] = incoming;
            samples[frame]               = static_cast<Sample>(delayed * gain_);
        }

        writeIndex_ = (writeIndex_ + 1) % delay_[0].size();
        ++frameClock_;
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
