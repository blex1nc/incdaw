#include "engine/dsp/effects/BeatGate.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace incdaw::engine::dsp {
namespace {

using Gate = BeatGateEffect;

/// The parameter table. The two curves are contiguous runs of sixteen, which
/// is what lets a view walk them and a preset carry a whole gesture.
constexpr EffectParameter descriptors[] = {
    {Gate::mix,          "Mix",        0.0,   1.0, 0.0, false},
    {Gate::timeAmount,   "Time",       0.0,   1.0, 1.0, false},
    {Gate::volumeAmount, "Volume",     0.0,   1.0, 1.0, false},
    {Gate::smoothingMs,  "Smoothing",  0.0, 200.0, 6.0, false},
    {Gate::bars,         "Bars",       1.0,   4.0, 1.0, true},

    {Gate::timeBase +  0, "T1",  0.0, 1.0, 0.0, false},
    {Gate::timeBase +  1, "T2",  0.0, 1.0, 0.0, false},
    {Gate::timeBase +  2, "T3",  0.0, 1.0, 0.0, false},
    {Gate::timeBase +  3, "T4",  0.0, 1.0, 0.0, false},
    {Gate::timeBase +  4, "T5",  0.0, 1.0, 0.0, false},
    {Gate::timeBase +  5, "T6",  0.0, 1.0, 0.0, false},
    {Gate::timeBase +  6, "T7",  0.0, 1.0, 0.0, false},
    {Gate::timeBase +  7, "T8",  0.0, 1.0, 0.0, false},
    {Gate::timeBase +  8, "T9",  0.0, 1.0, 0.0, false},
    {Gate::timeBase +  9, "T10", 0.0, 1.0, 0.0, false},
    {Gate::timeBase + 10, "T11", 0.0, 1.0, 0.0, false},
    {Gate::timeBase + 11, "T12", 0.0, 1.0, 0.0, false},
    {Gate::timeBase + 12, "T13", 0.0, 1.0, 0.0, false},
    {Gate::timeBase + 13, "T14", 0.0, 1.0, 0.0, false},
    {Gate::timeBase + 14, "T15", 0.0, 1.0, 0.0, false},
    {Gate::timeBase + 15, "T16", 0.0, 1.0, 0.0, false},

    {Gate::volumeBase +  0, "V1",  0.0, 1.0, 1.0, false},
    {Gate::volumeBase +  1, "V2",  0.0, 1.0, 1.0, false},
    {Gate::volumeBase +  2, "V3",  0.0, 1.0, 1.0, false},
    {Gate::volumeBase +  3, "V4",  0.0, 1.0, 1.0, false},
    {Gate::volumeBase +  4, "V5",  0.0, 1.0, 1.0, false},
    {Gate::volumeBase +  5, "V6",  0.0, 1.0, 1.0, false},
    {Gate::volumeBase +  6, "V7",  0.0, 1.0, 1.0, false},
    {Gate::volumeBase +  7, "V8",  0.0, 1.0, 1.0, false},
    {Gate::volumeBase +  8, "V9",  0.0, 1.0, 1.0, false},
    {Gate::volumeBase +  9, "V10", 0.0, 1.0, 1.0, false},
    {Gate::volumeBase + 10, "V11", 0.0, 1.0, 1.0, false},
    {Gate::volumeBase + 11, "V12", 0.0, 1.0, 1.0, false},
    {Gate::volumeBase + 12, "V13", 0.0, 1.0, 1.0, false},
    {Gate::volumeBase + 13, "V14", 0.0, 1.0, 1.0, false},
    {Gate::volumeBase + 14, "V15", 0.0, 1.0, 1.0, false},
    {Gate::volumeBase + 15, "V16", 0.0, 1.0, 1.0, false},
};

constexpr std::size_t descriptorCount = std::size(descriptors);
static_assert(descriptorCount == 5 + beatGatePoints * 2);

constexpr std::size_t firstTimeIndex   = 5;
constexpr std::size_t firstVolumeIndex = firstTimeIndex + beatGatePoints;

} // namespace

double beatGateCurveAt(const double points[beatGatePoints], double phase) noexcept
{
    double wrapped = phase - std::floor(phase);
    if (!(wrapped >= 0.0 && wrapped < 1.0))
        wrapped = 0.0;

    const double exact = wrapped * static_cast<double>(beatGatePoints);
    const auto   index = std::min(static_cast<std::size_t>(exact), beatGatePoints - 1);
    const double frac  = exact - static_cast<double>(index);

    // The last point interpolates back to the first: the bar line is a loop,
    // and a discontinuity there is a click on every bar.
    const double here = points[index];
    const double next = points[(index + 1) % beatGatePoints];

    return here + (next - here) * frac;
}

BeatGateEffect::BeatGateEffect(const TempoMap* tempoMap) noexcept
    : BuiltinEffect(descriptors, descriptorCount), tempoMap_(tempoMap)
{
}

void BeatGateEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    const auto frames = static_cast<std::size_t>(maxHistorySeconds * sampleRate_) + 1;

    for (std::vector<float>& channel : history_)
        channel.assign(frames, 0.0f);

    historyWrite_   = 0;
    smoothedOffset_ = 0.0;
}

std::array<double, beatGatePoints> BeatGateEffect::timeCurve() const noexcept
{
    std::array<double, beatGatePoints> points{};
    for (std::size_t index = 0; index < beatGatePoints; ++index)
        points[index] = value(timeBase + static_cast<std::uint32_t>(index));

    return points;
}

std::array<double, beatGatePoints> BeatGateEffect::volumeCurve() const noexcept
{
    std::array<double, beatGatePoints> points{};
    for (std::size_t index = 0; index < beatGatePoints; ++index)
        points[index] = value(volumeBase + static_cast<std::uint32_t>(index));

    return points;
}

double BeatGateEffect::phaseFor(FramePosition position, double barCount) const noexcept
{
    if (tempoMap_ == nullptr)
        return 0.0;

    const Tick          tick      = tempoMap_->tickForFrame(position);
    const TimeSignature signature = tempoMap_->timeSignatureAtTick(tick);

    const double ticksPerBar = static_cast<double>(ticksPerQuarterNote)
                             * 4.0 * static_cast<double>(signature.numerator)
                             / static_cast<double>(signature.denominator);

    if (ticksPerBar <= 0.0)
        return 0.0;

    const double pattern = ticksPerBar * std::max(barCount, 1.0);
    const double within  = std::fmod(static_cast<double>(tick), pattern);

    return (within < 0.0 ? within + pattern : within) / pattern;
}

void BeatGateEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double wet = valueAt(0);

    // Mix at zero is untouched, and so is a graph with no tempo map: the
    // effect has nothing to sync to, and guessing a bar would be worse than
    // doing nothing.
    if (wet == 0.0 || tempoMap_ == nullptr || history_[0].empty())
        return;

    const double timeAmount   = valueAt(1);
    const double volumeAmount = valueAt(2);
    const double smoothing    = valueAt(3);
    const double barCount     = valueAt(4);

    std::array<double, beatGatePoints> timePoints{};
    std::array<double, beatGatePoints> volumePoints{};

    for (std::size_t index = 0; index < beatGatePoints; ++index) {
        timePoints[index]   = valueAt(firstTimeIndex + index);
        volumePoints[index] = valueAt(firstVolumeIndex + index);
    }

    const std::size_t ring     = history_[0].size();
    const std::size_t channels = std::min(context.output.channelCount(), maxChannels);
    const double      dry      = 1.0 - wet;

    // How far a bar is, in frames, at this block's tempo. The curve's offset
    // is written in bars, so this is what turns it into samples.
    const double tempo = tempoMap_->tempoAtFrame(context.playPosition);
    const Tick   tick  = tempoMap_->tickForFrame(context.playPosition);
    const TimeSignature signature = tempoMap_->timeSignatureAtTick(tick);

    const double beatsPerBar = 4.0 * static_cast<double>(signature.numerator)
                             / static_cast<double>(signature.denominator);
    const double barFrames = tempo > 0.0
                                 ? (60.0 / tempo) * beatsPerBar * sampleRate_
                                 : sampleRate_ * 2.0;

    const double smoothingCoefficient =
        smoothing > 0.0 ? std::exp(-1.0 / (smoothing * 0.001 * sampleRate_)) : 0.0;

    // A stopped transport hands every block the same position, so the phase
    // would stand still and the gate would freeze on one value. Advancing it
    // by hand keeps the pattern audible while auditioning.
    const bool advancing = context.playing;

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        for (std::size_t channel = 0; channel < channels; ++channel)
            history_[channel][historyWrite_] =
                context.output.channel(channel)[frame];

        const FramePosition position =
            context.playPosition + static_cast<FramePosition>(advancing ? frame : 0);

        const double phase = phaseFor(position, barCount);

        // The time curve is written in BARS behind the present, and the
        // present is the newest sample: you cannot read into the future.
        const double wanted =
            beatGateCurveAt(timePoints.data(), phase) * timeAmount * barFrames * barCount;

        smoothedOffset_ =
            wanted + smoothingCoefficient * (smoothedOffset_ - wanted);

        const double offset =
            std::clamp(smoothedOffset_, 0.0, static_cast<double>(ring - 2));

        const double readPosition =
            static_cast<double>(historyWrite_) + static_cast<double>(ring) - offset;

        const auto   readIndex = static_cast<std::size_t>(readPosition) % ring;
        const double frac      = readPosition - std::floor(readPosition);
        const std::size_t nextIndex = (readIndex + 1) % ring;

        const double gain =
            1.0 - volumeAmount
                      * (1.0 - std::clamp(beatGateCurveAt(volumePoints.data(), phase),
                                          0.0, 1.0));

        for (std::size_t channel = 0; channel < channels; ++channel) {
            const double a = static_cast<double>(history_[channel][readIndex]);
            const double b = static_cast<double>(history_[channel][nextIndex]);

            const double shifted = a + (b - a) * frac;
            const double dryValue = static_cast<double>(context.output.channel(channel)[frame]);

            context.output.channel(channel)[frame] =
                static_cast<Sample>(dryValue * dry + shifted * gain * wet);
        }

        historyWrite_ = (historyWrite_ + 1) % ring;
    }
}

} // namespace incdaw::engine::dsp
