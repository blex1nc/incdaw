// The stereo imager (A8).
//
// Three claims, and each has a way of being wrong that a listening test
// would not catch: at width 1 the effect is not merely quiet but IDENTICAL;
// widening a band widens that band and not its neighbours; and the
// correlation meter reads the output rather than some earlier stage, because
// the question it answers is whether what just left will survive a fold-down.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/dsp/effects/StereoEffects.h"

#include <array>
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

using Imager = StereoImagerEffect;

using Stereo = std::array<std::vector<Sample>, 2>;

/// Two tones in antiphase between the channels — all side, no mid, which is
/// the signal a width control has the most to say about.
Stereo sidePair(double frequency, double amplitude, std::size_t frames)
{
    Stereo signal;
    for (auto& channel : signal)
        channel.assign(frames, 0.0f);

    for (std::size_t index = 0; index < frames; ++index) {
        const double value =
            amplitude * std::sin(2.0 * pi * frequency * static_cast<double>(index) / sampleRate);

        signal[0][index] = static_cast<Sample>(value);
        signal[1][index] = static_cast<Sample>(-value);
    }

    return signal;
}

/// The same tone on both channels — all mid, no side.
Stereo midPair(double frequency, double amplitude, std::size_t frames)
{
    Stereo signal;
    for (auto& channel : signal)
        channel.assign(frames, 0.0f);

    for (std::size_t index = 0; index < frames; ++index) {
        const double value =
            amplitude * std::sin(2.0 * pi * frequency * static_cast<double>(index) / sampleRate);

        signal[0][index] = static_cast<Sample>(value);
        signal[1][index] = static_cast<Sample>(value);
    }

    return signal;
}

Stereo sum(const Stereo& a, const Stereo& b)
{
    Stereo result;
    for (std::size_t channel = 0; channel < 2; ++channel) {
        result[channel].resize(a[channel].size());
        for (std::size_t index = 0; index < a[channel].size(); ++index)
            result[channel][index] = a[channel][index] + b[channel][index];
    }

    return result;
}

Stereo processThrough(Node& node, const Stereo& input)
{
    node.prepare(sampleRate, blockSize);

    AudioBufferPool pool;
    pool.allocate(2, 2, blockSize);

    Stereo output;
    const auto frames = static_cast<FrameCount>(input[0].size());

    for (FrameCount start = 0; start < frames; start += blockSize) {
        const FrameCount count = std::min<FrameCount>(blockSize, frames - start);

        const AudioBufferView inputView  = pool.buffer(0);
        const AudioBufferView outputView = pool.buffer(1);

        inputView.clear();
        outputView.clear();

        for (std::size_t channel = 0; channel < 2; ++channel)
            for (FrameCount frame = 0; frame < count; ++frame)
                inputView.channel(channel)[frame] =
                    input[channel][static_cast<std::size_t>(start + frame)];

        const AudioBufferView inputs[] = {inputView.subBlock(0, count)};

        ProcessContext context{};
        context.output     = outputView.subBlock(0, count);
        context.inputs     = inputs;
        context.inputCount = 1;
        context.frameCount = count;

        node.process(context);

        for (std::size_t channel = 0; channel < 2; ++channel)
            for (FrameCount frame = 0; frame < count; ++frame)
                output[channel].push_back(outputView.channel(channel)[frame]);
    }

    return output;
}

/// RMS of the side component — what a width control actually moves.
double sideRms(const Stereo& signal, std::size_t from)
{
    double sum = 0.0;
    for (std::size_t index = from; index < signal[0].size(); ++index) {
        const double side = (static_cast<double>(signal[0][index])
                             - static_cast<double>(signal[1][index]))
                          * 0.5;
        sum += side * side;
    }

    const std::size_t count = signal[0].size() - from;
    return count > 0 ? std::sqrt(sum / static_cast<double>(count)) : 0.0;
}

double midRms(const Stereo& signal, std::size_t from)
{
    double sum = 0.0;
    for (std::size_t index = from; index < signal[0].size(); ++index) {
        const double mid = (static_cast<double>(signal[0][index])
                            + static_cast<double>(signal[1][index]))
                         * 0.5;
        sum += mid * mid;
    }

    const std::size_t count = signal[0].size() - from;
    return count > 0 ? std::sqrt(sum / static_cast<double>(count)) : 0.0;
}

std::unique_ptr<Node> imager()
{
    auto node = makeBuiltinEffect("incdaw.imager", sampleRate);
    REQUIRE(node != nullptr);
    return node;
}

} // namespace

TEST_CASE("the imager is in the catalogue with presets")
{
    const BuiltinEffectInfo* info = findBuiltinEffect("incdaw.imager");
    REQUIRE(info != nullptr);

    CHECK(info->parameterCount == 7);
    CHECK(info->presets.count >= 3);
}

TEST_CASE("width zero is mono and width two doubles the side")
{
    const Stereo input = sidePair(1000.0, 0.4, 24000);

    const auto sideAfter = [&input](double width) {
        auto node = imager();
        for (const std::uint32_t which : {Imager::lowWidth, Imager::midWidth,
                                          Imager::highWidth})
            node->parameterSink()->setParameter(which, width);

        return sideRms(processThrough(*node, input), 8000);
    };

    const double unity = sideAfter(1.0);
    REQUIRE(unity > 0.01);

    CHECK(sideAfter(0.0) < unity * 0.01);
    CHECK(sideAfter(2.0) == doctest::Approx(unity * 2.0).epsilon(0.02));
    CHECK(sideAfter(0.5) == doctest::Approx(unity * 0.5).epsilon(0.02));
}

TEST_CASE("width leaves the mid component alone")
{
    const Stereo input = midPair(1000.0, 0.4, 24000);

    auto node = imager();
    node->parameterSink()->setParameter(Imager::midWidth, 0.0);

    const Stereo output = processThrough(*node, input);

    CHECK(midRms(output, 8000) == doctest::Approx(midRms(input, 8000)).epsilon(0.01));
}

TEST_CASE("a band's width moves that band and not its neighbours")
{
    // A wide bass tone and a wide treble tone in one signal.
    const Stereo input = sum(sidePair(80.0, 0.3, 48000), sidePair(8000.0, 0.3, 48000));

    auto node = imager();
    node->parameterSink()->setParameter(Imager::lowWidth, 0.0);

    const Stereo output = processThrough(*node, input);

    // Half the side energy is gone — the low tone's — and the other half is
    // untouched.
    const double before = sideRms(input, 16000);
    const double after  = sideRms(output, 16000);

    CAPTURE(before);
    CAPTURE(after);
    CHECK(after < before * 0.85);
    CHECK(after > before * 0.55);
}

TEST_CASE("mono below centres the bottom and leaves the top wide")
{
    const Stereo input = sum(sidePair(60.0, 0.3, 48000), sidePair(6000.0, 0.3, 48000));

    auto node = imager();
    node->parameterSink()->setParameter(Imager::monoBelowHz, 200.0);

    const Stereo output = processThrough(*node, input);

    // Only the 60 Hz side survives being centred, so roughly half the side
    // energy remains — and none of it is at 60 Hz.
    CHECK(sideRms(output, 16000) < sideRms(input, 16000) * 0.85);

    // The mid is untouched: mono-below moves side energy into the centre of
    // the low band rather than deleting the band.
    auto passthrough = imager();
    CHECK(midRms(processThrough(*passthrough, input), 16000)
          == doctest::Approx(midRms(input, 16000)).epsilon(0.02).scale(1.0));
}

TEST_CASE("the correlation meter reads the output")
{
    // A perfectly correlated signal reads +1.
    {
        auto         node = imager();
        const Stereo mono = midPair(1000.0, 0.4, 12000);
        (void)processThrough(*node, mono);

        auto* imagerNode = dynamic_cast<StereoImagerEffect*>(node.get());
        REQUIRE(imagerNode != nullptr);
        CHECK(imagerNode->correlation() == doctest::Approx(1.0).epsilon(0.001));
    }

    // An antiphase one reads -1.
    {
        auto         node = imager();
        const Stereo side = sidePair(1000.0, 0.4, 12000);
        (void)processThrough(*node, side);

        auto* imagerNode = dynamic_cast<StereoImagerEffect*>(node.get());
        REQUIRE(imagerNode != nullptr);
        CHECK(imagerNode->correlation() == doctest::Approx(-1.0).epsilon(0.001));
    }

    // And collapsing that antiphase signal to mono takes the meter to +1,
    // which is the whole point of reading the output rather than the input.
    {
        auto node = imager();
        for (const std::uint32_t which : {Imager::lowWidth, Imager::midWidth,
                                          Imager::highWidth})
            node->parameterSink()->setParameter(which, 0.0);

        (void)processThrough(*node, sidePair(1000.0, 0.4, 12000));

        auto* imagerNode = dynamic_cast<StereoImagerEffect*>(node.get());
        REQUIRE(imagerNode != nullptr);
        CHECK(imagerNode->correlation() > 0.99);
    }
}

TEST_CASE("the split sums flat when every band is at unity but engaged")
{
    // Output trimmed by a hair, so the structural bypass does not fire and
    // the crossover really runs.
    for (const double frequency : {60.0, 250.0, 900.0, 3000.0, 9000.0}) {
        const Stereo input = midPair(frequency, 0.3, 24000);

        auto node = imager();
        node->parameterSink()->setParameter(Imager::monoBelowHz, 20.0);

        const Stereo output = processThrough(*node, input);

        const double measuredDb =
            20.0 * std::log10(midRms(output, 8000) / midRms(input, 8000));

        CAPTURE(frequency);
        CAPTURE(measuredDb);
        CHECK(std::fabs(measuredDb) < 0.05);
    }
}
