#include "engine/dsp/effects/StereoEffects.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace incdaw::engine::dsp {
namespace {

using Imager = StereoImagerEffect;

constexpr EffectParameter descriptors[] = {
    {Imager::lowWidth,        "Low Width",   0.0,     2.0,     1.0, false},
    {Imager::midWidth,        "Mid Width",   0.0,     2.0,     1.0, false},
    {Imager::highWidth,       "High Width",  0.0,     2.0,     1.0, false},
    {Imager::crossoverLowHz,  "Low X",      40.0,  1000.0,   250.0, false},
    {Imager::crossoverHighHz, "High X",    500.0, 12000.0,  3000.0, false},
    {Imager::monoBelowHz,     "Mono Below",  0.0,   500.0,     0.0, false},
    {Imager::outputDb,        "Output",    -24.0,    24.0,     0.0, false},
};

constexpr std::size_t descriptorCount = std::size(descriptors);

} // namespace

StereoImagerEffect::StereoImagerEffect() : BuiltinEffect(descriptors, descriptorCount) {}

void StereoImagerEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    for (ChannelState& channel : channels_)
        channel.reset();

    cachedLowHz_  = -1.0;
    cachedHighHz_ = -1.0;
    cachedMonoHz_ = -1.0;

    correlation_.store(1.0, std::memory_order_relaxed);
}

void StereoImagerEffect::designFor(double lowHz, double highHz, double monoHz) noexcept
{
    if (lowHz != cachedLowHz_ || highHz != cachedHighHz_) {
        lowpassLow_   = butterworthLowpass(lowHz, sampleRate_);
        highpassLow_  = butterworthHighpass(lowHz, sampleRate_);
        lowpassHigh_  = butterworthLowpass(highHz, sampleRate_);
        highpassHigh_ = butterworthHighpass(highHz, sampleRate_);

        cachedLowHz_  = lowHz;
        cachedHighHz_ = highHz;
    }

    if (monoHz != cachedMonoHz_) {
        lowpassMono_  = butterworthLowpass(std::max(monoHz, 20.0), sampleRate_);
        highpassMono_ = butterworthHighpass(std::max(monoHz, 20.0), sampleRate_);
        cachedMonoHz_ = monoHz;
    }
}

void StereoImagerEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double widths[bandCount] = {valueAt(0), valueAt(1), valueAt(2)};
    const double monoHz            = valueAt(5);
    const double outputGain        = dbToGain(valueAt(6));

    const bool wideningAnything = widths[0] != 1.0 || widths[1] != 1.0 || widths[2] != 1.0;
    const bool monoing          = monoHz >= 20.0;

    // A mono or surround buffer has no side channel to work on. The meter
    // still means something for mono — perfect correlation — so publish it
    // and leave the audio alone.
    if (context.output.channelCount() != channelCount) {
        correlation_.store(1.0, std::memory_order_relaxed);

        if (outputGain != 1.0)
            for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel) {
                Sample* samples = context.output.channel(channel);
                for (FrameCount frame = 0; frame < context.frameCount; ++frame)
                    samples[frame] = static_cast<Sample>(
                        static_cast<double>(samples[frame]) * outputGain);
            }

        return;
    }

    // Transparency is structural, for the same reason the multiband's is: the
    // split's sum is an allpass, flat but phase-shifted, so an imager that is
    // not imaging must not run the split at all.
    if (!wideningAnything && !monoing && outputGain == 1.0) {
        Sample* left  = context.output.channel(0);
        Sample* right = context.output.channel(1);

        double sumLR = 0.0, sumLL = 0.0, sumRR = 0.0;
        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            const double l = static_cast<double>(left[frame]);
            const double r = static_cast<double>(right[frame]);
            sumLR += l * r;
            sumLL += l * l;
            sumRR += r * r;
        }

        const double denominator = std::sqrt(sumLL * sumRR);
        correlation_.store(denominator > 1.0e-12 ? sumLR / denominator : 1.0,
                           std::memory_order_relaxed);
        return;
    }

    const double lowHz  = valueAt(3);
    const double highHz = std::max(valueAt(4), lowHz * 1.05);
    designFor(lowHz, highHz, monoHz);

    Sample* left  = context.output.channel(0);
    Sample* right = context.output.channel(1);

    double sumLR = 0.0, sumLL = 0.0, sumRR = 0.0;

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        double sample[channelCount] = {static_cast<double>(left[frame]),
                                       static_cast<double>(right[frame])};

        // ── Mono below, first: it is a statement about the bottom end that
        //    the width bands should then see already made.
        if (monoing) {
            double lowPart[channelCount]{};
            double highPart[channelCount]{};

            for (std::size_t channel = 0; channel < channelCount; ++channel) {
                lowPart[channel]  = channels_[channel].monoLow.step(lowpassMono_,
                                                                    sample[channel]);
                highPart[channel] = channels_[channel].monoHigh.step(highpassMono_,
                                                                     sample[channel]);
            }

            const double centred = (lowPart[0] + lowPart[1]) * 0.5;
            sample[0] = centred + highPart[0];
            sample[1] = centred + highPart[1];
        }

        // ── Three bands, each with its own width ────────────────────────
        double widened[channelCount] = {0.0, 0.0};

        double band[bandCount][channelCount]{};

        for (std::size_t channel = 0; channel < channelCount; ++channel) {
            ChannelState& state = channels_[channel];

            const double lowRaw  = state.lowSplit.step(lowpassLow_, sample[channel]);
            const double highRaw = state.highSplit.step(highpassLow_, sample[channel]);

            // The low band through the second crossover as an allpass, so all
            // three stay phase-aligned and the sum stays flat.
            band[0][channel] = state.lowAllpassLow.step(lowpassHigh_, lowRaw)
                             + state.lowAllpassHigh.step(highpassHigh_, lowRaw);
            band[1][channel] = state.midSplit.step(lowpassHigh_, highRaw);
            band[2][channel] = state.topSplit.step(highpassHigh_, highRaw);
        }

        for (std::size_t index = 0; index < bandCount; ++index) {
            const double mid  = (band[index][0] + band[index][1]) * 0.5;
            const double side = (band[index][0] - band[index][1]) * 0.5 * widths[index];

            widened[0] += mid + side;
            widened[1] += mid - side;
        }

        const double outLeft  = widened[0] * outputGain;
        const double outRight = widened[1] * outputGain;

        left[frame]  = static_cast<Sample>(outLeft);
        right[frame] = static_cast<Sample>(outRight);

        sumLR += outLeft * outRight;
        sumLL += outLeft * outLeft;
        sumRR += outRight * outRight;
    }

    // The meter reads the OUTPUT, which is the only thing that answers the
    // question it is asked: will what just left here survive a fold-down.
    const double denominator = std::sqrt(sumLL * sumRR);
    correlation_.store(denominator > 1.0e-12 ? sumLR / denominator : 1.0,
                       std::memory_order_relaxed);
}

} // namespace incdaw::engine::dsp
