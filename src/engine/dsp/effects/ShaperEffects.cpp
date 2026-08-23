#include "engine/dsp/effects/ShaperEffects.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>

namespace incdaw::engine::dsp {
namespace {

using Shaper = WaveshaperEffect;

constexpr double pi = std::numbers::pi;

constexpr EffectParameter descriptors[] = {
    {Shaper::driveDb,    "Drive",       0.0,  36.0, 0.0, false},
    {Shaper::mix,        "Mix",         0.0,   1.0, 1.0, false},
    {Shaper::outputDb,   "Output",    -24.0,  24.0, 0.0, false},
    {Shaper::oversample, "Oversample",  0.0,   2.0, 1.0, true},

    {Shaper::pointBase + 0, "P1", -1.0, 1.0, -1.0,   false},
    {Shaper::pointBase + 1, "P2", -1.0, 1.0, -0.75,  false},
    {Shaper::pointBase + 2, "P3", -1.0, 1.0, -0.5,   false},
    {Shaper::pointBase + 3, "P4", -1.0, 1.0, -0.25,  false},
    {Shaper::pointBase + 4, "P5", -1.0, 1.0,  0.0,   false},
    {Shaper::pointBase + 5, "P6", -1.0, 1.0,  0.25,  false},
    {Shaper::pointBase + 6, "P7", -1.0, 1.0,  0.5,   false},
    {Shaper::pointBase + 7, "P8", -1.0, 1.0,  0.75,  false},
    {Shaper::pointBase + 8, "P9", -1.0, 1.0,  1.0,   false},
};

constexpr std::size_t descriptorCount = std::size(descriptors);
constexpr std::size_t firstPointIndex = 4;

static_assert(descriptorCount == firstPointIndex + shaperPointCount);

/// The halfband's coefficients: a windowed sinc at a quarter of the rate the
/// filter runs at, Blackman window. Designed once, in the constructor.
void designHalfband(std::array<double, halfbandTaps>& coefficients) noexcept
{
    constexpr int centre = halfbandTaps / 2;

    double sum = 0.0;

    for (int index = 0; index < halfbandTaps; ++index) {
        const int    offset = index - centre;
        const double sinc   = offset == 0 ? 0.5
                                          : std::sin(pi * static_cast<double>(offset) * 0.5)
                                                / (pi * static_cast<double>(offset));

        const double phase = 2.0 * pi * static_cast<double>(index)
                           / static_cast<double>(halfbandTaps - 1);
        const double window = 0.42 - 0.5 * std::cos(phase) + 0.08 * std::cos(2.0 * phase);

        coefficients[static_cast<std::size_t>(index)] = sinc * window;
        sum += coefficients[static_cast<std::size_t>(index)];
    }

    // Unity at DC, so an oversampled path does not change the level.
    if (sum != 0.0)
        for (double& coefficient : coefficients)
            coefficient /= sum;
}

} // namespace

double shaperCurveAt(const double points[shaperPointCount], double x) noexcept
{
    const double clamped = std::clamp(x, -1.0, 1.0);

    const double span    = 2.0 / static_cast<double>(shaperPointCount - 1);
    const double exact   = (clamped + 1.0) / span;
    const auto   segment = std::min(static_cast<std::size_t>(exact), shaperPointCount - 2);
    const double t       = exact - static_cast<double>(segment);

    // Catmull-Rom needs a point either side of the segment, and the two at the
    // ends do not exist. They are EXTRAPOLATED along the end slope rather than
    // duplicated: a duplicate would halve the tangent at the first and last
    // point, which bends the identity curve away from a straight line right
    // where a shaper is least forgiving about it.
    constexpr auto last = static_cast<std::ptrdiff_t>(shaperPointCount) - 1;

    const auto at = [points](std::ptrdiff_t index) {
        if (index < 0)
            return 2.0 * points[0] - points[1];

        if (index > last)
            return 2.0 * points[static_cast<std::size_t>(last)]
                 - points[static_cast<std::size_t>(last - 1)];

        return points[static_cast<std::size_t>(index)];
    };

    const auto   here = static_cast<std::ptrdiff_t>(segment);
    const double p0 = at(here - 1), p1 = at(here), p2 = at(here + 1), p3 = at(here + 2);

    const double t2 = t * t;
    const double t3 = t2 * t;

    return 0.5 * ((2.0 * p1) + (-p0 + p2) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2
                  + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
}

double WaveshaperEffect::Halfband::step(const std::array<double, taps>& coefficients,
                                        double input) noexcept
{
    history[static_cast<std::size_t>(cursor)] = input;

    double sum = 0.0;

    // Half of a halfband's taps are exactly zero. Skipping them is not an
    // optimisation detail — it is why this filter is affordable per sample.
    for (int tap = 0; tap < taps; ++tap) {
        const int offset = tap - centre;
        if (offset != 0 && (offset % 2) == 0)
            continue;

        int index = cursor - tap;
        if (index < 0)
            index += taps;

        sum += coefficients[static_cast<std::size_t>(tap)]
             * history[static_cast<std::size_t>(index)];
    }

    cursor = (cursor + 1) % taps;
    return sum;
}

WaveshaperEffect::WaveshaperEffect() : BuiltinEffect(descriptors, descriptorCount)
{
    designHalfband(coefficients_);
}

void WaveshaperEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    for (auto& channel : stages_)
        for (Stage& stage : channel)
            stage.reset();

    tableValid_ = false;
}

void WaveshaperEffect::rebuildTableIfNeeded() noexcept
{
    std::array<double, shaperPointCount> points{};
    bool changed = !tableValid_;

    for (std::size_t index = 0; index < shaperPointCount; ++index) {
        points[index] = valueAt(firstPointIndex + index);
        changed       = changed || points[index] != cachedPoints_[index];
    }

    if (!changed)
        return;

    // Bounded work, no allocation: a thousand spline evaluations, and only
    // when a point has actually moved.
    for (std::size_t index = 0; index <= tableSize; ++index) {
        const double x = -1.0 + 2.0 * static_cast<double>(index)
                                    / static_cast<double>(tableSize);
        table_[index] = shaperCurveAt(points.data(), x);
    }

    cachedPoints_ = points;
    tableValid_   = true;
}

double WaveshaperEffect::shapeThroughTable(double x) const noexcept
{
    const double clamped = std::clamp(x, -1.0, 1.0);
    const double exact   = (clamped + 1.0) * 0.5 * static_cast<double>(tableSize);

    const auto   index = std::min(static_cast<std::size_t>(exact), tableSize - 1);
    const double frac  = exact - static_cast<double>(index);

    return table_[index] + (table_[index + 1] - table_[index]) * frac;
}

void WaveshaperEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double drive     = dbToGain(valueAt(0));
    const double wet       = valueAt(1);
    const double output    = dbToGain(valueAt(2));
    const auto   stageCount = static_cast<int>(valueAt(3));

    // The transparent case is the identity curve at unity, and it is answered
    // structurally: an oversampler is not a bit-exact path, so the only way
    // to pass the signal through untouched is not to run one.
    bool identity = drive == 1.0 && wet == 1.0 && output == 1.0;

    for (std::size_t index = 0; index < shaperPointCount && identity; ++index)
        identity = valueAt(firstPointIndex + index) == shaperPointX(index);

    if (identity)
        return;

    rebuildTableIfNeeded();

    const std::size_t channels = std::min(context.output.channelCount(), maxChannels);
    const double      dry      = 1.0 - wet;

    for (std::size_t channel = 0; channel < channels; ++channel) {
        Sample* samples = context.output.channel(channel);
        auto&   stages  = stages_[channel];

        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            const double input = static_cast<double>(samples[frame]);
            double       shaped = 0.0;

            if (stageCount <= 0) {
                shaped = shapeThroughTable(input * drive);
            } else if (stageCount == 1) {
                const double a = 2.0 * stages[0].up.step(coefficients_, input);
                const double b = 2.0 * stages[0].up.step(coefficients_, 0.0);

                (void)stages[0].down.step(coefficients_, shapeThroughTable(a * drive));
                shaped = 2.0 * stages[0].down.step(coefficients_,
                                                   shapeThroughTable(b * drive));
            } else {
                // 4x: the second stage runs inside the first, on each of the
                // two samples the first produced.
                const double outer[2] = {2.0 * stages[0].up.step(coefficients_, input),
                                         2.0 * stages[0].up.step(coefficients_, 0.0)};

                double folded[2] = {0.0, 0.0};

                for (int half = 0; half < 2; ++half) {
                    const double a =
                        2.0 * stages[1].up.step(coefficients_, outer[half]);
                    const double b = 2.0 * stages[1].up.step(coefficients_, 0.0);

                    (void)stages[1].down.step(coefficients_, shapeThroughTable(a * drive));
                    folded[half] = 2.0 * stages[1].down.step(
                        coefficients_, shapeThroughTable(b * drive));
                }

                (void)stages[0].down.step(coefficients_, folded[0]);
                shaped = 2.0 * stages[0].down.step(coefficients_, folded[1]);
            }

            samples[frame] =
                static_cast<Sample>((input * dry + shaped * wet) * output);
        }
    }
}

} // namespace incdaw::engine::dsp
