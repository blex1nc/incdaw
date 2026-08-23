#include "engine/dsp/effects/SpaceEffects.h"
#include "engine/dsp/effects/EffectRegistry.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace incdaw::engine::dsp {

namespace {

constexpr EffectParameter delayParameters[] = {
    {DelayEffect::timeMs,   "Time",     1.0, DelayEffect::maxTimeMs, 250.0, false},
    {DelayEffect::feedback, "Feedback", 0.0, 0.95,                     0.3, false},
    {DelayEffect::mix,      "Mix",      0.0, 1.0,                      0.3, false},
};

constexpr EffectParameter reverbParameters[] = {
    {ReverbEffect::size,    "Size",    0.2, 1.5, 0.8, false},
    {ReverbEffect::damping, "Damping", 0.0, 1.0, 0.4, false},
    {ReverbEffect::mix,     "Mix",     0.0, 1.0, 0.3, false},
};

/// Comb and allpass tunings in frames at 48 kHz, scaled to the session rate
/// in prepare. Mutually prime-ish lengths, the classic recipe; the right
/// channel adds a small offset for decorrelation.
constexpr FrameCount combTunings48k[ReverbEffect::combCount]       = {1557, 1617, 1491, 1422};
constexpr FrameCount allpassTunings48k[ReverbEffect::allpassCount] = {225, 556};
constexpr FrameCount stereoSpread48k                               = 23;

} // namespace

// ── DelayEffect ──────────────────────────────────────────────────────────────

DelayEffect::DelayEffect() : BuiltinEffect(delayParameters, 3) {}

void DelayEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate;
    capacity_   = static_cast<FrameCount>(maxTimeMs * 0.001 * sampleRate) + 1;
    writeIndex_ = 0;

    for (auto& line : lines_)
        line.assign(static_cast<std::size_t>(capacity_), Sample{0});
}

void DelayEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    if (capacity_ <= 0)
        return;

    const double wetness  = valueAt(2);
    const double feedbackGain = valueAt(1);
    const auto   delayFrames  = std::clamp<FrameCount>(
        static_cast<FrameCount>(valueAt(0) * 0.001 * sampleRate_), 1, capacity_ - 1);

    const std::size_t channels =
        std::min<std::size_t>(context.output.channelCount(), maxChannels);

    for (std::size_t channel = 0; channel < channels; ++channel) {
        Sample*    samples = context.output.channel(channel);
        Sample*    line    = lines_[channel].data();
        FrameCount write   = writeIndex_;

        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            FrameCount read = write - delayFrames;
            if (read < 0)
                read += capacity_;

            const double dry     = static_cast<double>(samples[frame]);
            const double delayed = static_cast<double>(line[read]);

            // The line records dry input plus what feeds back around.
            line[write] = static_cast<Sample>(dry + delayed * feedbackGain);

            if (wetness > 0.0)
                samples[frame] = static_cast<Sample>(dry + delayed * wetness);

            if (++write >= capacity_)
                write = 0;
        }
    }

    writeIndex_ = (writeIndex_ + context.frameCount) % capacity_;
}

// ── ReverbEffect ─────────────────────────────────────────────────────────────

ReverbEffect::ReverbEffect() : BuiltinEffect(reverbParameters, 3) {}

void ReverbEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;

    const double scale = sampleRate / 48000.0;

    for (std::size_t channel = 0; channel < maxChannels; ++channel) {
        const FrameCount spread =
            channel == 1 ? static_cast<FrameCount>(stereoSpread48k * scale) : 0;

        for (std::size_t comb = 0; comb < combCount; ++comb) {
            const auto frames =
                static_cast<FrameCount>(static_cast<double>(combTunings48k[comb]) * scale)
                + spread;
            combs_[channel][comb].line.assign(static_cast<std::size_t>(frames), Sample{0});
            combs_[channel][comb].index = 0;
            combs_[channel][comb].store = 0.0;
        }

        for (std::size_t allpass = 0; allpass < allpassCount; ++allpass) {
            const auto frames =
                static_cast<FrameCount>(static_cast<double>(allpassTunings48k[allpass]) * scale)
                + spread;
            allpasses_[channel][allpass].line.assign(static_cast<std::size_t>(frames),
                                                     Sample{0});
            allpasses_[channel][allpass].index = 0;
        }
    }
}

void ReverbEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double wetness = valueAt(2);
    if (wetness <= 0.0)
        return;   // bit-exact dry: silence in, the tail state still decays later

    // Size maps to comb feedback: bigger room, longer tail.
    const double roomFeedback = std::clamp(0.7 + valueAt(0) * 0.18, 0.0, 0.98);
    const double damp         = std::clamp(valueAt(1), 0.0, 1.0) * 0.8;
    constexpr double allpassG = 0.5;

    const std::size_t channels =
        std::min<std::size_t>(context.output.channelCount(), maxChannels);

    for (std::size_t channel = 0; channel < channels; ++channel) {
        Sample* samples = context.output.channel(channel);

        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            const double dry   = static_cast<double>(samples[frame]);
            const double input = dry * 0.2;   // classic input attenuation into the network

            double wet = 0.0;

            for (std::size_t combIndex = 0; combIndex < combCount; ++combIndex) {
                Comb& comb = combs_[channel][combIndex];

                const double out = static_cast<double>(comb.line[comb.index]);
                wet += out;

                // One-pole lowpass in the feedback path: air absorbs highs.
                comb.store = out * (1.0 - damp) + comb.store * damp;

                comb.line[comb.index] =
                    static_cast<Sample>(input + comb.store * roomFeedback);

                if (++comb.index >= comb.line.size())
                    comb.index = 0;
            }

            for (std::size_t allpassIndex = 0; allpassIndex < allpassCount; ++allpassIndex) {
                Allpass& allpass = allpasses_[channel][allpassIndex];

                const double buffered = static_cast<double>(allpass.line[allpass.index]);
                const double output   = -wet + buffered;

                allpass.line[allpass.index] =
                    static_cast<Sample>(wet + buffered * allpassG);

                if (++allpass.index >= allpass.line.size())
                    allpass.index = 0;

                wet = output;
            }

            samples[frame] = static_cast<Sample>(dry + wet * wetness);
        }
    }
}

// ── Registrar ────────────────────────────────────────────────────────────────

void registerSpaceEffects(std::vector<EffectCatalogueEntry>& rows)
{
    addEffect(rows, "incdaw.delay",  "Delay",
              [](SampleRate) { return std::make_unique<DelayEffect>(); });
    addEffect(rows, "incdaw.reverb", "Reverb",
              [](SampleRate) { return std::make_unique<ReverbEffect>(); });
}

} // namespace incdaw::engine::dsp
