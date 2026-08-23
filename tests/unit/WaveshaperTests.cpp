// The drawable waveshaper (A9).
//
// Three claims. The curve the audio thread applies is the curve the control
// points describe — and the same one a view would draw, because both go
// through `shaperCurveAt`. The identity curve is a bit-exact pass. And the
// oversampling is not decoration: a drawn curve has corners, corners make
// harmonics above Nyquist, and the difference between shaping at 1x and at
// 4x is measurable in the spectrum.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/Fft.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/dsp/effects/ShaperEffects.h"

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
using namespace incdaw::engine::dsp;

namespace {

constexpr FrameCount blockSize  = 256;
constexpr double     sampleRate = 48000.0;
constexpr double     pi         = std::numbers::pi;

using Shaper = WaveshaperEffect;

std::vector<Sample> processThrough(Node& node, const std::vector<Sample>& input)
{
    node.prepare(sampleRate, blockSize);

    AudioBufferPool pool;
    pool.allocate(2, 1, blockSize);

    std::vector<Sample> output;
    output.reserve(input.size());

    const auto frames = static_cast<FrameCount>(input.size());

    for (FrameCount start = 0; start < frames; start += blockSize) {
        const FrameCount count = std::min<FrameCount>(blockSize, frames - start);

        const AudioBufferView inputView  = pool.buffer(0);
        const AudioBufferView outputView = pool.buffer(1);

        inputView.clear();
        outputView.clear();

        for (FrameCount frame = 0; frame < count; ++frame)
            inputView.channel(0)[frame] = input[static_cast<std::size_t>(start + frame)];

        const AudioBufferView inputs[] = {inputView.subBlock(0, count)};

        ProcessContext context{};
        context.output     = outputView.subBlock(0, count);
        context.inputs     = inputs;
        context.inputCount = 1;
        context.frameCount = count;

        node.process(context);

        for (FrameCount frame = 0; frame < count; ++frame)
            output.push_back(outputView.channel(0)[frame]);
    }

    return output;
}

/// A hard-clipping curve: flat past two thirds, straight in between. Corners,
/// which is exactly what oversampling is for.
void setHardClip(ParameterSink& sink)
{
    const double points[shaperPointCount] = {-0.7, -0.7, -0.55, -0.28, 0.0,
                                             0.28, 0.55, 0.7,   0.7};

    for (std::size_t index = 0; index < shaperPointCount; ++index)
        sink.setParameter(Shaper::pointBase + static_cast<std::uint32_t>(index),
                          points[index]);
}

std::vector<Sample> sine(double frequency, double amplitude, std::size_t frames)
{
    std::vector<Sample> samples(frames, 0.0f);
    for (std::size_t index = 0; index < frames; ++index)
        samples[index] = static_cast<Sample>(
            amplitude * std::sin(2.0 * pi * frequency * static_cast<double>(index) / sampleRate));

    return samples;
}

std::vector<double> magnitudes(std::vector<Sample> samples)
{
    const std::size_t length = samples.size();
    for (std::size_t index = 0; index < length; ++index) {
        const double window = 0.5 - 0.5 * std::cos(2.0 * pi * static_cast<double>(index)
                                                   / static_cast<double>(length));
        samples[index] = static_cast<Sample>(static_cast<double>(samples[index]) * window);
    }

    Fft fft;
    fft.setSize(length);

    std::vector<float> imaginary(length, 0.0f);
    fft.forward(samples.data(), imaginary.data());

    std::vector<double> result(length / 2 + 1, 0.0);
    for (std::size_t bin = 0; bin < result.size(); ++bin)
        result[bin] = std::hypot(static_cast<double>(samples[bin]),
                                 static_cast<double>(imaginary[bin]));

    return result;
}

} // namespace

TEST_CASE("the shaper is in the catalogue, points and all")
{
    const BuiltinEffectInfo* info = findBuiltinEffect("incdaw.shaper");
    REQUIRE(info != nullptr);

    CHECK(info->parameterCount == 4 + shaperPointCount);
    CHECK(info->presets.count >= 3);

    // The points are contiguous, which is what lets a curve view walk them.
    for (std::size_t index = 0; index < shaperPointCount; ++index) {
        const std::uint32_t id = Shaper::pointBase + static_cast<std::uint32_t>(index);

        bool found = false;
        for (std::size_t which = 0; which < info->parameterCount; ++which)
            found = found || info->parameters[which].id == id;

        CAPTURE(index);
        CHECK(found);
    }
}

TEST_CASE("the curve passes through its own control points")
{
    const double points[shaperPointCount] = {-0.9, -0.6, -0.2, -0.05, 0.1,
                                             0.35, 0.55, 0.8,  0.95};

    for (std::size_t index = 0; index < shaperPointCount; ++index) {
        CAPTURE(index);
        CHECK(shaperCurveAt(points, shaperPointX(index))
              == doctest::Approx(points[index]).epsilon(1e-9));
    }
}

TEST_CASE("the identity points describe the identity")
{
    double points[shaperPointCount]{};
    for (std::size_t index = 0; index < shaperPointCount; ++index)
        points[index] = shaperPointX(index);

    for (double x = -1.0; x <= 1.0; x += 0.017) {
        CAPTURE(x);
        CHECK(shaperCurveAt(points, x) == doctest::Approx(x).epsilon(1e-9));
    }

    // And outside the range it clamps rather than running away.
    CHECK(shaperCurveAt(points, 4.0) == doctest::Approx(1.0));
    CHECK(shaperCurveAt(points, -4.0) == doctest::Approx(-1.0));
}

TEST_CASE("what the effect applies is what the curve function draws")
{
    auto node = makeBuiltinEffect("incdaw.shaper", sampleRate);
    REQUIRE(node != nullptr);

    ParameterSink* sink = node->parameterSink();
    setHardClip(*sink);
    sink->setParameter(Shaper::oversample, 0.0);   // 1x: a static transfer

    const double points[shaperPointCount] = {-0.7, -0.7, -0.55, -0.28, 0.0,
                                             0.28, 0.55, 0.7,   0.7};

    // A slow ramp across the whole input range.
    constexpr std::size_t frames = 2048;
    std::vector<Sample>   ramp(frames, 0.0f);
    for (std::size_t index = 0; index < frames; ++index)
        ramp[index] = static_cast<Sample>(-1.0 + 2.0 * static_cast<double>(index)
                                                    / static_cast<double>(frames - 1));

    const std::vector<Sample> output = processThrough(*node, ramp);

    double worst = 0.0;
    for (std::size_t index = 0; index < frames; ++index) {
        const double expected = shaperCurveAt(points, static_cast<double>(ramp[index]));
        worst = std::max(worst, std::fabs(static_cast<double>(output[index]) - expected));
    }

    CAPTURE(worst);
    CHECK(worst < 2.0e-3);   // the table's resolution, and nothing else
}

TEST_CASE("drive scales into the curve and mix blends back out")
{
    const std::vector<Sample> ramp = {-0.5f, -0.25f, 0.0f, 0.25f, 0.5f};

    const double points[shaperPointCount] = {-0.7, -0.7, -0.55, -0.28, 0.0,
                                             0.28, 0.55, 0.7,   0.7};

    auto node = makeBuiltinEffect("incdaw.shaper", sampleRate);
    ParameterSink* sink = node->parameterSink();
    setHardClip(*sink);
    sink->setParameter(Shaper::oversample, 0.0);
    sink->setParameter(Shaper::driveDb, 6.0);
    sink->setParameter(Shaper::mix, 0.5);

    const std::vector<Sample> output = processThrough(*node, ramp);

    const double drive = std::pow(10.0, 6.0 / 20.0);

    for (std::size_t index = 0; index < ramp.size(); ++index) {
        const double dry = static_cast<double>(ramp[index]);
        const double wet = shaperCurveAt(points, dry * drive);

        CAPTURE(index);
        CHECK(static_cast<double>(output[index])
              == doctest::Approx(dry * 0.5 + wet * 0.5).epsilon(0.01).scale(1.0));
    }
}

TEST_CASE("oversampling is the difference between distortion and gravel")
{
    constexpr double      frequency = 5000.0;
    constexpr std::size_t frames    = 16384;

    const auto aliasEnergy = [&](double oversample) {
        auto node = makeBuiltinEffect("incdaw.shaper", sampleRate);
        ParameterSink* sink = node->parameterSink();
        setHardClip(*sink);
        sink->setParameter(Shaper::oversample, oversample);
        sink->setParameter(Shaper::driveDb, 12.0);

        const std::vector<double> spectrum =
            magnitudes(processThrough(*node, sine(frequency, 0.8, frames)));

        const double binHz = sampleRate / static_cast<double>(frames);

        double fundamental = 0.0;
        double alias       = 0.0;

        for (std::size_t bin = 4; bin < spectrum.size(); ++bin) {
            const double hz = static_cast<double>(bin) * binHz;

            // Odd harmonics of a symmetric curve, and their reflections back
            // under Nyquist, are what the curve legitimately produces.
            bool legitimate = false;
            for (int harmonic = 1; harmonic <= 9; harmonic += 2) {
                const double ideal = frequency * static_cast<double>(harmonic);
                if (std::fabs(hz - ideal) < binHz * 8.0)
                    legitimate = true;
            }

            if (legitimate) {
                fundamental = std::max(fundamental, spectrum[bin]);
                continue;
            }

            alias += spectrum[bin] * spectrum[bin];
        }

        return std::sqrt(alias) / std::max(fundamental, 1e-12);
    };

    const double none = aliasEnergy(0.0);
    const double four = aliasEnergy(2.0);

    CAPTURE(none);
    CAPTURE(four);

    // Four times over is at least six decibels cleaner, and in practice far
    // more.
    CHECK(four < none * 0.5);
}

TEST_CASE("a curve that is moved mid-stream is picked up")
{
    auto node = makeBuiltinEffect("incdaw.shaper", sampleRate);
    ParameterSink* sink = node->parameterSink();
    sink->setParameter(Shaper::oversample, 0.0);

    const std::vector<Sample> flat(512, 0.5f);

    // The identity first: the structural bypass, so the signal is untouched.
    CHECK(static_cast<double>(processThrough(*node, flat).back())
          == doctest::Approx(0.5).epsilon(1e-9));

    // Then a point is dragged, and the very next block follows it.
    sink->setParameter(Shaper::pointBase + 6, 0.2);   // x = +0.5 now maps to 0.2

    const std::vector<Sample> shaped = processThrough(*node, flat);
    CHECK(static_cast<double>(shaped.back()) == doctest::Approx(0.2).epsilon(0.01));
}
