// The multiband compressor (A6).
//
// The compressor law is the single-band one, already proven in
// BuiltinEffectTests. What is new — and what this file is about — is the
// CROSSOVER: three bands that do not sum back to the signal they were split
// from make an effect that colours everything it touches whether it is doing
// anything or not.
//
// So the tests are, in order of importance: the network sums flat, the drawn
// response agrees with the filter that is actually running, a band that is
// doing nothing is doing nothing, and solo and bypass mean what they say.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/dsp/effects/MultibandEffects.h"

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

using Multiband = MultibandCompressorEffect;

std::vector<Sample> sine(double frequency, double amplitude, std::size_t frames)
{
    std::vector<Sample> samples(frames, 0.0f);
    for (std::size_t index = 0; index < frames; ++index)
        samples[index] = static_cast<Sample>(
            amplitude * std::sin(2.0 * pi * frequency * static_cast<double>(index) / sampleRate));

    return samples;
}

/// Runs one mono signal through the node under the real Node contract.
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

double peakOf(const std::vector<Sample>& samples, std::size_t from)
{
    double peak = 0.0;
    for (std::size_t index = from; index < samples.size(); ++index)
        peak = std::max(peak, std::fabs(static_cast<double>(samples[index])));

    return peak;
}

/// A multiband whose split is RUNNING but whose bands are not reducing:
/// ratio above 1 with the threshold at 0 dBFS, which a quiet signal never
/// reaches. This is how the crossover can be measured on its own.
std::unique_ptr<Node> engagedButUnity()
{
    auto node = makeBuiltinEffect("incdaw.multiband", sampleRate);
    REQUIRE(node != nullptr);

    ParameterSink* sink = node->parameterSink();
    REQUIRE(sink != nullptr);

    for (std::size_t band = 0; band < Multiband::bandCount; ++band) {
        sink->setParameter(Multiband::bandParameter(band, Multiband::bandRatio), 4.0);
        sink->setParameter(Multiband::bandParameter(band, Multiband::bandThresholdDb), 0.0);
    }

    return node;
}

} // namespace

TEST_CASE("the multiband is in the catalogue with a control per band")
{
    const BuiltinEffectInfo* info = findBuiltinEffect("incdaw.multiband");
    REQUIRE(info != nullptr);

    CHECK(info->parameterCount == 24);
    CHECK(info->presets.count >= 3);

    for (std::size_t band = 0; band < Multiband::bandCount; ++band)
        for (const Multiband::BandOffset offset :
             {Multiband::bandThresholdDb, Multiband::bandRatio, Multiband::bandAttackMs,
              Multiband::bandReleaseMs, Multiband::bandMakeupDb, Multiband::bandBypass,
              Multiband::bandSolo}) {
            const std::uint32_t id = Multiband::bandParameter(band, offset);

            bool found = false;
            for (std::size_t index = 0; index < info->parameterCount; ++index)
                found = found || info->parameters[index].id == id;

            CAPTURE(band);
            CAPTURE(static_cast<int>(offset));
            CHECK(found);
        }
}

TEST_CASE("the crossover network sums flat across the spectrum")
{
    // The drawn response first: cheap, and it says whether the design is
    // right before any audio is rendered.
    for (double frequency = 20.0; frequency < 18000.0; frequency *= 1.12) {
        const double summed = multibandSumMagnitudeDb(200.0, 2500.0, sampleRate, frequency);

        CAPTURE(frequency);
        CAPTURE(summed);
        CHECK(std::fabs(summed) < 0.05);
    }
}

TEST_CASE("the picture cannot disagree with the filter that is running")
{
    // Every frequency is measured through the real node, and held to what
    // multibandSumMagnitudeDb draws for it. A design that drifts from its own
    // plot fails here.
    for (const double frequency : {60.0, 150.0, 200.0, 320.0, 800.0, 2000.0, 2500.0,
                                   3500.0, 8000.0}) {
        const std::vector<Sample> input = sine(frequency, 0.25, 24000);

        std::unique_ptr<Node>     node   = engagedButUnity();
        const std::vector<Sample> output = processThrough(*node, input);

        // Skip the filters' settling time; measure over a long tail.
        const double inputRms  = rmsOf(input, 8000);
        const double outputRms = rmsOf(output, 8000);

        const double measuredDb = 20.0 * std::log10(outputRms / inputRms);
        const double drawnDb =
            multibandSumMagnitudeDb(200.0, 2500.0, sampleRate, frequency);

        CAPTURE(frequency);
        CAPTURE(measuredDb);
        CAPTURE(drawnDb);

        CHECK(std::fabs(measuredDb - drawnDb) < 0.05);
        CHECK(std::fabs(measuredDb) < 0.1);
    }
}

TEST_CASE("a band reduces by exactly the law the single-band compressor uses")
{
    constexpr double frequency = 1000.0;   // squarely in the mid band
    constexpr double amplitude = 0.5;
    constexpr double threshold = -20.0;
    constexpr double ratio     = 4.0;

    auto node = makeBuiltinEffect("incdaw.multiband", sampleRate);
    REQUIRE(node != nullptr);
    ParameterSink* sink = node->parameterSink();

    sink->setParameter(Multiband::bandParameter(1, Multiband::bandThresholdDb), threshold);
    sink->setParameter(Multiband::bandParameter(1, Multiband::bandRatio), ratio);
    // A fast attack and a slow release, so the envelope settles at the gain
    // the PEAK asks for. With a release near the tone's own period the
    // detector would ride each half-cycle instead, which is a different
    // (and correct) behaviour that a single steady-state number cannot
    // describe.
    sink->setParameter(Multiband::bandParameter(1, Multiband::bandAttackMs), 1.0);
    sink->setParameter(Multiband::bandParameter(1, Multiband::bandReleaseMs), 1000.0);
    sink->setParameter(Multiband::bandParameter(1, Multiband::bandSolo), 1.0);

    const std::vector<Sample> input  = sine(frequency, amplitude, 48000);
    const std::vector<Sample> output = processThrough(*node, input);

    // The reference: the same gain computer, written out here.
    const double peakDb      = 20.0 * std::log10(amplitude);
    const double overDb      = peakDb - threshold;
    const double reductionDb = overDb > 0.0 ? overDb * (1.0 / ratio - 1.0) : 0.0;
    const double expected    = amplitude * std::pow(10.0, reductionDb / 20.0);

    // A soloed band is the band alone. 1 kHz sits inside the mid band but not
    // dead centre, so the crossovers take about a fifth of a decibel off it —
    // hence three per cent rather than a hard equality.
    CAPTURE(expected);
    CHECK(peakOf(output, 24000) == doctest::Approx(expected).epsilon(0.03));
}

TEST_CASE("solo leaves one band and mutes the rest")
{
    const std::vector<Sample> low = sine(60.0, 0.4, 24000);

    auto node = makeBuiltinEffect("incdaw.multiband", sampleRate);
    ParameterSink* sink = node->parameterSink();
    sink->setParameter(Multiband::bandParameter(2, Multiband::bandSolo), 1.0);

    const std::vector<Sample> output = processThrough(*node, low);

    // A 60 Hz tone with only the high band soloed is essentially gone.
    CHECK(peakOf(output, 8000) < peakOf(low, 8000) * 0.01);

    auto lowSolo = makeBuiltinEffect("incdaw.multiband", sampleRate);
    lowSolo->parameterSink()->setParameter(
        Multiband::bandParameter(0, Multiband::bandSolo), 1.0);

    const std::vector<Sample> kept = processThrough(*lowSolo, low);
    CHECK(peakOf(kept, 8000) == doctest::Approx(peakOf(low, 8000)).epsilon(0.02));
}

TEST_CASE("a bypassed band is not compressed, and the others still are")
{
    const std::vector<Sample> tone = sine(1000.0, 0.5, 48000);

    const auto peakWith = [&tone](double bypassed) {
        auto node = makeBuiltinEffect("incdaw.multiband", sampleRate);
        ParameterSink* sink = node->parameterSink();

        sink->setParameter(Multiband::bandParameter(1, Multiband::bandThresholdDb), -30.0);
        sink->setParameter(Multiband::bandParameter(1, Multiband::bandRatio), 10.0);
        sink->setParameter(Multiband::bandParameter(1, Multiband::bandAttackMs), 1.0);
        sink->setParameter(Multiband::bandParameter(1, Multiband::bandBypass), bypassed);

        return peakOf(processThrough(*node, tone), 24000);
    };

    const double compressed = peakWith(0.0);
    const double bypassed   = peakWith(1.0);

    CHECK(compressed < bypassed * 0.5);
    CHECK(bypassed == doctest::Approx(peakOf(tone, 24000)).epsilon(0.02));
}

TEST_CASE("the crossovers stay ordered even when asked not to be")
{
    // A user dragging the low crossover above the high one must not produce a
    // network with a negative-width band.
    auto node = makeBuiltinEffect("incdaw.multiband", sampleRate);
    ParameterSink* sink = node->parameterSink();

    sink->setParameter(Multiband::crossoverLowHz, 1000.0);
    sink->setParameter(Multiband::crossoverHighHz, 500.0);
    sink->setParameter(Multiband::bandParameter(0, Multiband::bandRatio), 4.0);

    const std::vector<Sample> output = processThrough(*node, sine(800.0, 0.3, 12000));

    for (const Sample sample : output)
        REQUIRE(std::isfinite(sample));
}

TEST_CASE("the meter reports what each band reduced")
{
    MultibandCompressorEffect effect;
    effect.prepare(sampleRate, blockSize);

    effect.setParameter(Multiband::bandParameter(1, Multiband::bandThresholdDb), -30.0);
    effect.setParameter(Multiband::bandParameter(1, Multiband::bandRatio), 8.0);
    effect.setParameter(Multiband::bandParameter(1, Multiband::bandAttackMs), 1.0);

    (void)processThrough(effect, sine(1000.0, 0.6, 24000));

    CHECK(effect.gainReductionDb(1) > 6.0);
    CHECK(effect.gainReductionDb(0) == doctest::Approx(0.0).epsilon(0.001));
}

// ── The de-esser ─────────────────────────────────────────────────────────────

namespace {

std::unique_ptr<Node> deEsser(double frequency, double threshold, double ratio, double range)
{
    auto node = makeBuiltinEffect("incdaw.deesser", sampleRate);
    REQUIRE(node != nullptr);

    ParameterSink* sink = node->parameterSink();
    sink->setParameter(DeEsserEffect::frequencyHz, frequency);
    sink->setParameter(DeEsserEffect::thresholdDb, threshold);
    sink->setParameter(DeEsserEffect::ratio, ratio);
    sink->setParameter(DeEsserEffect::rangeDb, range);
    sink->setParameter(DeEsserEffect::attackMs, 0.5);
    sink->setParameter(DeEsserEffect::releaseMs, 500.0);

    return node;
}

std::vector<Sample> mix(const std::vector<Sample>& a, const std::vector<Sample>& b)
{
    std::vector<Sample> sum(a.size(), 0.0f);
    for (std::size_t index = 0; index < a.size(); ++index)
        sum[index] = a[index] + b[index];

    return sum;
}

} // namespace

TEST_CASE("the de-esser is in the catalogue with presets")
{
    const BuiltinEffectInfo* info = findBuiltinEffect("incdaw.deesser");
    REQUIRE(info != nullptr);

    CHECK(info->parameterCount == 8);
    CHECK(info->presets.count >= 3);
}

TEST_CASE("split band leaves the body of the voice alone")
{
    const std::vector<Sample> body = sine(220.0, 0.4, 48000);

    std::unique_ptr<Node>     node   = deEsser(6000.0, -40.0, 10.0, 20.0);
    const std::vector<Sample> output = processThrough(*node, body);

    // Nothing above 6 kHz to detect, so nothing happens — and the crossover's
    // two halves sum back to what went in.
    CHECK(peakOf(output, 24000) == doctest::Approx(peakOf(body, 24000)).epsilon(0.01));
}

TEST_CASE("split band pulls sibilance down and nothing else")
{
    const std::vector<Sample> voice = mix(sine(220.0, 0.4, 48000), sine(9000.0, 0.4, 48000));

    std::unique_ptr<Node>     node   = deEsser(6000.0, -30.0, 8.0, 20.0);
    const std::vector<Sample> output = processThrough(*node, voice);

    // The sum is quieter than the input, because half of it was reduced...
    CHECK(peakOf(output, 24000) < peakOf(voice, 24000) * 0.95);

    // ...and what is left is dominated by the untouched low tone.
    const std::vector<Sample> lowAlone = sine(220.0, 0.4, 48000);
    CHECK(peakOf(output, 24000) > peakOf(lowAlone, 24000) * 0.9);
}

TEST_CASE("the range caps how much an s can be pulled back")
{
    constexpr double amplitude = 0.8;
    constexpr double range     = 6.0;

    // Well above the crossover, so the tone is essentially all in the band
    // being reduced and the measurement is of the cap rather than of the
    // filter's skirt.
    const std::vector<Sample> sibilance = sine(14000.0, amplitude, 48000);

    // Threshold far below and a hard ratio: without the cap this would be
    // reduced by more than forty decibels.
    std::unique_ptr<Node>     node   = deEsser(4000.0, -50.0, 20.0, range);
    const std::vector<Sample> output = processThrough(*node, sibilance);

    const double expected = amplitude * std::pow(10.0, -range / 20.0);

    CAPTURE(expected);
    CHECK(peakOf(output, 24000) == doctest::Approx(expected).epsilon(0.03));
}

TEST_CASE("wideband ducks the whole signal, split band does not")
{
    const std::vector<Sample> voice = mix(sine(220.0, 0.4, 48000), sine(9000.0, 0.4, 48000));

    const auto lowContentAfter = [&voice](int mode) {
        std::unique_ptr<Node> node = deEsser(6000.0, -30.0, 8.0, 20.0);
        node->parameterSink()->setParameter(DeEsserEffect::mode, static_cast<double>(mode));

        const std::vector<Sample> output = processThrough(*node, voice);

        // The low tone's own contribution, isolated by its RMS over a stretch
        // long enough for the 9 kHz partner to average out of the estimate.
        return rmsOf(output, 24000);
    };

    CHECK(lowContentAfter(DeEsserEffect::wideband)
          < lowContentAfter(DeEsserEffect::splitBand) * 0.95);
}

TEST_CASE("listen hands over what the detector hears")
{
    const std::vector<Sample> voice = mix(sine(220.0, 0.4, 48000), sine(9000.0, 0.4, 48000));

    auto node = makeBuiltinEffect("incdaw.deesser", sampleRate);
    node->parameterSink()->setParameter(DeEsserEffect::listen, 1.0);
    node->parameterSink()->setParameter(DeEsserEffect::frequencyHz, 3000.0);

    const std::vector<Sample> output = processThrough(*node, voice);

    // The 220 Hz half is gone — four octaves below a fourth-order highpass is
    // ninety decibels down — and the 9 kHz half is through at full level.
    CHECK(peakOf(output, 24000) == doctest::Approx(0.4).epsilon(0.03));
}

TEST_CASE("the de-esser's split sums flat when it is not reducing")
{
    for (const double frequency : {200.0, 3000.0, 6000.0, 9000.0, 14000.0}) {
        const std::vector<Sample> input = sine(frequency, 0.25, 24000);

        // Ratio above 1 so the split runs, threshold at 0 dBFS so it never
        // reduces: the crossover on its own.
        auto node = makeBuiltinEffect("incdaw.deesser", sampleRate);
        node->parameterSink()->setParameter(DeEsserEffect::ratio, 4.0);
        node->parameterSink()->setParameter(DeEsserEffect::thresholdDb, 0.0);

        const std::vector<Sample> output = processThrough(*node, input);

        const double measuredDb = 20.0 * std::log10(rmsOf(output, 8000) / rmsOf(input, 8000));

        CAPTURE(frequency);
        CAPTURE(measuredDb);
        CHECK(std::fabs(measuredDb) < 0.05);
    }
}
