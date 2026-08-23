// The parametric EQ (A10).
//
// The property the three-band EQ established and this one inherits: the curve
// a view draws and the filter the audio thread runs come from the SAME
// design. A picture that disagrees with the filter is worse than no picture,
// so the central test here measures the rendered response of the real node
// and holds it to what `parametricMagnitudeDb` draws — band by band, type by
// type.
//
// The reference for a single band is a straight-line biquad loop written out
// here, so a coefficient that drifts from the cookbook fails.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/dsp/effects/ParametricEq.h"

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

using Eq = ParametricEqEffect;

std::vector<Sample> sine(double frequency, double amplitude, std::size_t frames)
{
    std::vector<Sample> samples(frames, 0.0f);
    for (std::size_t index = 0; index < frames; ++index)
        samples[index] = static_cast<Sample>(
            amplitude * std::sin(2.0 * pi * frequency * static_cast<double>(index) / sampleRate));

    return samples;
}

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

double rmsOf(const std::vector<Sample>& samples, std::size_t from)
{
    double sum = 0.0;
    for (std::size_t index = from; index < samples.size(); ++index)
        sum += static_cast<double>(samples[index]) * static_cast<double>(samples[index]);

    const std::size_t count = samples.size() - from;
    return count > 0 ? std::sqrt(sum / static_cast<double>(count)) : 0.0;
}

void setBand(ParameterSink& sink, std::size_t band, ParametricBandType type,
             double frequency, double gainDb, double q)
{
    sink.setParameter(Eq::bandParameter(band, Eq::bandType), static_cast<double>(type));
    sink.setParameter(Eq::bandParameter(band, Eq::bandFrequency), frequency);
    sink.setParameter(Eq::bandParameter(band, Eq::bandGainDb), gainDb);
    sink.setParameter(Eq::bandParameter(band, Eq::bandQ), q);
}

} // namespace

TEST_CASE("the parametric EQ is in the catalogue with four controls per band")
{
    const BuiltinEffectInfo* info = findBuiltinEffect("incdaw.eqp");
    REQUIRE(info != nullptr);

    CHECK(info->parameterCount == 1 + parametricBandCount * 4);
    CHECK(info->presets.count >= 3);

    for (std::size_t band = 0; band < parametricBandCount; ++band)
        for (const Eq::BandOffset offset :
             {Eq::bandType, Eq::bandFrequency, Eq::bandGainDb, Eq::bandQ}) {
            const std::uint32_t id = Eq::bandParameter(band, offset);

            bool found = false;
            for (std::size_t index = 0; index < info->parameterCount; ++index)
                found = found || info->parameters[index].id == id;

            CAPTURE(band);
            CHECK(found);
        }
}

TEST_CASE("a band at zero gain, and a band switched off, are the identity")
{
    for (const ParametricBandType type :
         {ParametricBandType::off, ParametricBandType::lowShelf, ParametricBandType::peak,
          ParametricBandType::highShelf}) {
        const ParametricBand band{type, 1000.0, 0.0, 1.0};
        const BiquadCoefficients c = designParametricBand(band, sampleRate);

        CAPTURE(parametricBandTypeName(type));
        CHECK(c.b0 == doctest::Approx(1.0));
        CHECK(c.b1 == doctest::Approx(0.0));
        CHECK(c.b2 == doctest::Approx(0.0));
        CHECK(c.a1 == doctest::Approx(0.0));
        CHECK(c.a2 == doctest::Approx(0.0));
    }
}

TEST_CASE("a peak's gain at its own frequency is the gain that was asked for")
{
    for (const double gain : {-12.0, -6.0, 3.0, 9.0, 18.0}) {
        std::array<ParametricBand, parametricBandCount> bands{};
        bands[3] = {ParametricBandType::peak, 1000.0, gain, 2.0};

        CAPTURE(gain);
        CHECK(parametricMagnitudeDb(bands, sampleRate, 1000.0)
              == doctest::Approx(gain).epsilon(0.02));
    }
}

TEST_CASE("a shelf reaches its gain well past the corner")
{
    std::array<ParametricBand, parametricBandCount> bands{};
    bands[0] = {ParametricBandType::lowShelf, 200.0, 6.0, 0.707};

    CHECK(parametricMagnitudeDb(bands, sampleRate, 25.0) == doctest::Approx(6.0).epsilon(0.05));
    CHECK(parametricMagnitudeDb(bands, sampleRate, 200.0) == doctest::Approx(3.0).epsilon(0.1));
    CHECK(std::fabs(parametricMagnitudeDb(bands, sampleRate, 8000.0)) < 0.1);

    std::array<ParametricBand, parametricBandCount> high{};
    high[0] = {ParametricBandType::highShelf, 4000.0, -6.0, 0.707};

    CHECK(parametricMagnitudeDb(high, sampleRate, 18000.0)
          == doctest::Approx(-6.0).epsilon(0.1));
}

TEST_CASE("a highpass is down three decibels at its corner, and a notch is a hole")
{
    std::array<ParametricBand, parametricBandCount> bands{};
    bands[0] = {ParametricBandType::highPass, 500.0, 0.0, 0.707};

    CHECK(parametricMagnitudeDb(bands, sampleRate, 500.0) == doctest::Approx(-3.0).epsilon(0.05));
    CHECK(parametricMagnitudeDb(bands, sampleRate, 50.0) < -35.0);
    CHECK(std::fabs(parametricMagnitudeDb(bands, sampleRate, 8000.0)) < 0.3);

    std::array<ParametricBand, parametricBandCount> notch{};
    notch[0] = {ParametricBandType::notch, 1000.0, 0.0, 8.0};

    CHECK(parametricMagnitudeDb(notch, sampleRate, 1000.0) < -60.0);
    CHECK(std::fabs(parametricMagnitudeDb(notch, sampleRate, 200.0)) < 0.5);
}

TEST_CASE("the plotted response is the response the EQ actually applies")
{
    // Four bands of different types at once, so the test is of the whole
    // chain rather than of one cookbook formula.
    auto node = makeBuiltinEffect("incdaw.eqp", sampleRate);
    REQUIRE(node != nullptr);

    ParameterSink* sink = node->parameterSink();
    setBand(*sink, 0, ParametricBandType::highPass, 80.0, 0.0, 0.707);
    setBand(*sink, 2, ParametricBandType::peak, 400.0, -8.0, 2.5);
    setBand(*sink, 5, ParametricBandType::peak, 3000.0, 6.0, 1.2);
    setBand(*sink, 7, ParametricBandType::highShelf, 9000.0, 4.0, 0.707);

    auto* eq = dynamic_cast<ParametricEqEffect*>(node.get());
    REQUIRE(eq != nullptr);

    // `bands()` reads what the audio thread reads, so this really is the same
    // design on both sides of the comparison.
    const std::array<ParametricBand, parametricBandCount> bands = eq->bands();

    for (const double frequency : {60.0, 120.0, 400.0, 900.0, 3000.0, 6000.0, 12000.0}) {
        const std::vector<Sample> input = sine(frequency, 0.25, 32768);

        auto fresh = makeBuiltinEffect("incdaw.eqp", sampleRate);
        ParameterSink* freshSink = fresh->parameterSink();
        setBand(*freshSink, 0, ParametricBandType::highPass, 80.0, 0.0, 0.707);
        setBand(*freshSink, 2, ParametricBandType::peak, 400.0, -8.0, 2.5);
        setBand(*freshSink, 5, ParametricBandType::peak, 3000.0, 6.0, 1.2);
        setBand(*freshSink, 7, ParametricBandType::highShelf, 9000.0, 4.0, 0.707);

        const std::vector<Sample> output = processThrough(*fresh, input);

        const double measuredDb =
            20.0 * std::log10(rmsOf(output, 12000) / rmsOf(input, 12000));
        const double drawnDb = parametricMagnitudeDb(bands, sampleRate, frequency);

        CAPTURE(frequency);
        CAPTURE(measuredDb);
        CAPTURE(drawnDb);
        CHECK(std::fabs(measuredDb - drawnDb) < 0.1);
    }
}

TEST_CASE("one band matches a reference biquad loop written out by hand")
{
    const ParametricBand band{ParametricBandType::peak, 1500.0, 9.0, 3.0};

    auto node = makeBuiltinEffect("incdaw.eqp", sampleRate);
    ParameterSink* sink = node->parameterSink();
    setBand(*sink, 4, band.type, band.frequency, band.gainDb, band.q);

    std::vector<Sample> input(4096, 0.0f);
    std::uint32_t       state = 0x2468ACE0u;
    for (Sample& sample : input) {
        state  = state * 1664525u + 1013904223u;
        sample = static_cast<Sample>((static_cast<double>(state) / 4294967295.0) * 0.5 - 0.25);
    }

    const std::vector<Sample> output = processThrough(*node, input);

    // The reference: the cookbook peak, written out again, run as a plain
    // transposed direct form II loop.
    const double w0    = 2.0 * pi * band.frequency / sampleRate;
    const double alpha = std::sin(w0) / (2.0 * band.q);
    const double A     = std::pow(10.0, band.gainDb / 40.0);
    const double a0    = 1.0 + alpha / A;

    const double b0 = (1.0 + alpha * A) / a0;
    const double b1 = (-2.0 * std::cos(w0)) / a0;
    const double b2 = (1.0 - alpha * A) / a0;
    const double a1 = (-2.0 * std::cos(w0)) / a0;
    const double a2 = (1.0 - alpha / A) / a0;

    double z1 = 0.0, z2 = 0.0;

    for (std::size_t index = 0; index < input.size(); ++index) {
        const double x = static_cast<double>(input[index]);
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;

        CAPTURE(index);
        REQUIRE(static_cast<double>(output[index])
                == doctest::Approx(y).epsilon(1e-5).scale(1.0));
    }
}

TEST_CASE("the output trim is applied after the bands")
{
    auto node = makeBuiltinEffect("incdaw.eqp", sampleRate);
    node->parameterSink()->setParameter(Eq::outputDb, -6.0);

    const std::vector<Sample> input  = sine(1000.0, 0.4, 8192);
    const std::vector<Sample> output = processThrough(*node, input);

    CHECK(rmsOf(output, 2000)
          == doctest::Approx(rmsOf(input, 2000) * std::pow(10.0, -6.0 / 20.0)).epsilon(0.005));
}

TEST_CASE("a band moved mid-stream is redesigned, and nothing blows up")
{
    auto node = makeBuiltinEffect("incdaw.eqp", sampleRate);
    ParameterSink* sink = node->parameterSink();

    AudioBufferPool pool;
    pool.allocate(2, 1, blockSize);

    node->prepare(sampleRate, blockSize);

    for (int block = 0; block < 64; ++block) {
        setBand(*sink, 3, ParametricBandType::peak,
                200.0 + 200.0 * static_cast<double>(block),
                12.0 - 0.3 * static_cast<double>(block), 0.5 + 0.2 * static_cast<double>(block));

        const AudioBufferView inputView  = pool.buffer(0);
        const AudioBufferView outputView = pool.buffer(1);

        inputView.clear();
        outputView.clear();

        for (FrameCount frame = 0; frame < blockSize; ++frame)
            inputView.channel(0)[frame] = static_cast<Sample>(
                0.3 * std::sin(2.0 * pi * 440.0 * static_cast<double>(frame) / sampleRate));

        const AudioBufferView inputs[] = {inputView};

        ProcessContext context{};
        context.output     = outputView;
        context.inputs     = inputs;
        context.inputCount = 1;
        context.frameCount = blockSize;

        node->process(context);

        for (FrameCount frame = 0; frame < blockSize; ++frame)
            REQUIRE(std::isfinite(outputView.channel(0)[frame]));
    }
}
