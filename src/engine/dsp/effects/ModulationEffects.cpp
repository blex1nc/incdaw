#include "engine/dsp/effects/ModulationEffects.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine::dsp {
namespace {

constexpr double twoPi = 6.28318530717958647692;

/// Smallest power of two holding `frames`, for mask-wrapped delay lines.
std::size_t ringSizeFor(double frames) noexcept
{
    std::size_t size = 16;
    while (size < static_cast<std::size_t>(frames) + 4)
        size *= 2;
    return size;
}

/// Linear-interpolated read `delayFrames` behind `write`.
Sample readDelayed(const std::vector<Sample>& line, std::size_t mask, std::size_t write,
                   double delayFrames) noexcept
{
    const double position = static_cast<double>(write) - delayFrames;
    const double floored  = std::floor(position);
    const double fraction = position - floored;

    const auto   index = static_cast<std::size_t>(static_cast<long long>(floored));
    const Sample a     = line[index & mask];
    const Sample b     = line[(index + 1) & mask];

    return a + static_cast<Sample>(fraction) * (b - a);
}

} // namespace

// ── ChorusEffect ──────────────────────────────────────────────────────────────

namespace {
constexpr EffectParameter chorusParameters[] = {
    {ChorusEffect::rateHz,  "Rate",  0.05, 5.0,  0.8, false},
    {ChorusEffect::depthMs, "Depth", 0.0,  10.0, 3.0, false},
    {ChorusEffect::mix,     "Mix",   0.0,  1.0,  0.0, false},
};
} // namespace

ChorusEffect::ChorusEffect()
    : BuiltinEffect(chorusParameters, std::size(chorusParameters)) {}

void ChorusEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    const std::size_t size = ringSizeFor((centreMs + maxDepthMs + 2.0) * 0.001 * sampleRate_);
    for (auto& line : line_) {
        line.assign(size, Sample{0});
    }
    mask_  = size - 1;
    write_ = 0;
    phase_ = 0.0;
}

void ChorusEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double rate  = valueAt(0);
    const double depth = valueAt(1) * 0.001 * sampleRate_;
    const double wet   = valueAt(2);
    const double step  = twoPi * rate / sampleRate_;
    const double base  = centreMs * 0.001 * sampleRate_;

    const std::size_t channels = std::min<std::size_t>(context.output.channelCount(), maxChannels);
    if (channels == 0 || line_[0].empty())
        return;

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        for (std::size_t channel = 0; channel < channels; ++channel) {
            Sample* samples = context.output.channel(channel);
            const Sample dry = samples[frame];

            line_[channel][write_ & mask_] = dry;

            if (wet > 0.0) {
                // Quadrature between the channels keeps the image moving
                // instead of pumping.
                const double angle = phase_ + (channel == 1 ? twoPi * 0.25 : 0.0);
                const double delay = base + depth * 0.5 * (1.0 + std::sin(angle));

                // Additive: the dry path stays put and the moving voice joins
                // it, scaled to keep the blend near constant power.
                const Sample voice = readDelayed(line_[channel], mask_, write_, delay);
                samples[frame]     = dry + static_cast<Sample>(wet * 0.7071) * voice;
            }
        }

        ++write_;
        phase_ += step;
        if (phase_ > twoPi)
            phase_ -= twoPi;
    }
}

// ── FlangerEffect ─────────────────────────────────────────────────────────────

namespace {
constexpr EffectParameter flangerParameters[] = {
    {FlangerEffect::rateHz,   "Rate",     0.05,  2.0,  0.25, false},
    {FlangerEffect::depthMs,  "Depth",    0.0,   5.0,  2.0,  false},
    {FlangerEffect::feedback, "Feedback", -0.95, 0.95, 0.5,  false},
    {FlangerEffect::mix,      "Mix",      0.0,   1.0,  0.0,  false},
};
} // namespace

FlangerEffect::FlangerEffect()
    : BuiltinEffect(flangerParameters, std::size(flangerParameters)) {}

void FlangerEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    const std::size_t size = ringSizeFor((baseMs + maxDepthMs + 2.0) * 0.001 * sampleRate_);
    for (auto& line : line_)
        line.assign(size, Sample{0});
    for (Sample& held : held_)
        held = Sample{0};
    mask_  = size - 1;
    write_ = 0;
    phase_ = 0.0;
}

void FlangerEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double rate     = valueAt(0);
    const double depth    = valueAt(1) * 0.001 * sampleRate_;
    const double feedback = valueAt(2);
    const double wet      = valueAt(3);
    const double step     = twoPi * rate / sampleRate_;
    const double base     = baseMs * 0.001 * sampleRate_;

    const std::size_t channels = std::min<std::size_t>(context.output.channelCount(), maxChannels);
    if (channels == 0 || line_[0].empty())
        return;

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        const double delay = base + depth * 0.5 * (1.0 + std::sin(phase_));

        for (std::size_t channel = 0; channel < channels; ++channel) {
            Sample* samples = context.output.channel(channel);
            const Sample dry = samples[frame];

            line_[channel][write_ & mask_] =
                dry + static_cast<Sample>(feedback) * held_[channel];

            if (wet > 0.0) {
                const Sample voice = readDelayed(line_[channel], mask_, write_, delay);
                held_[channel]     = voice;
                samples[frame]     = dry + static_cast<Sample>(wet) * voice;
            } else {
                held_[channel] = readDelayed(line_[channel], mask_, write_, delay);
            }
        }

        ++write_;
        phase_ += step;
        if (phase_ > twoPi)
            phase_ -= twoPi;
    }
}

// ── PhaserEffect ──────────────────────────────────────────────────────────────

namespace {
constexpr EffectParameter phaserParameters[] = {
    {PhaserEffect::rateHz,   "Rate",     0.05,  2.0,  0.4, false},
    {PhaserEffect::feedback, "Feedback", -0.95, 0.95, 0.3, false},
    {PhaserEffect::mix,      "Mix",      0.0,   1.0,  0.0, false},
};
} // namespace

PhaserEffect::PhaserEffect()
    : BuiltinEffect(phaserParameters, std::size(phaserParameters)) {}

void PhaserEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    for (auto& channelStates : state_)
        for (double& state : channelStates)
            state = 0.0;
    for (Sample& held : held_)
        held = Sample{0};
    phase_ = 0.0;
}

void PhaserEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double rate     = valueAt(0);
    const double feedback = valueAt(1);
    const double wet      = valueAt(2);
    const double step     = twoPi * rate / sampleRate_;

    const std::size_t channels = std::min<std::size_t>(context.output.channelCount(), maxChannels);
    if (channels == 0 || wet <= 0.0) {
        // Silent path still advances nothing: the allpass chain only matters
        // when audible, and the dry signal is already in place.
        return;
    }

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        // Sweep the allpass corner between 200 Hz and 2 kHz, geometrically.
        const double sweep  = 0.5 * (1.0 + std::sin(phase_));
        const double corner = 200.0 * std::pow(10.0, sweep);   // 200 → 2000 Hz
        const double tangent = std::tan(3.14159265358979323846 * corner / sampleRate_);
        const double coefficient = (tangent - 1.0) / (tangent + 1.0);

        for (std::size_t channel = 0; channel < channels; ++channel) {
            Sample* samples = context.output.channel(channel);
            const double dry = static_cast<double>(samples[frame]);

            double value = dry + feedback * static_cast<double>(held_[channel]);
            for (std::size_t stage = 0; stage < stages; ++stage) {
                const double output = coefficient * value + state_[channel][stage];
                state_[channel][stage] = value - coefficient * output;
                value                  = output;
            }

            held_[channel] = static_cast<Sample>(value);
            samples[frame] = static_cast<Sample>(dry + wet * value);
        }

        phase_ += step;
        if (phase_ > twoPi)
            phase_ -= twoPi;
    }
}

} // namespace incdaw::engine::dsp
