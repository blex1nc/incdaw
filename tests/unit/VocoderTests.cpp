// The vocoder (A12).
//
// A vocoder is only doing its job if the OUTPUT's spectrum follows the
// MODULATOR's while its excitation comes from the carrier. So the tests feed
// the two apart — a bright saw as carrier, a narrow tone as modulator — and
// check that energy appears where the modulator put it and nowhere else.
//
// The other half of the work is the wiring: a sidechain edge now lands on
// anything that implements KeyedEffect rather than on a named list of uids,
// and the compressor and the vocoder both do.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/Fft.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/dsp/effects/DynamicsEffects.h"
#include "engine/dsp/effects/Vocoder.h"

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
using namespace incdaw::engine::dsp;

namespace {

constexpr FrameCount blockSize  = 512;
constexpr double     sampleRate = 48000.0;
constexpr double     pi         = std::numbers::pi;

using Voc = VocoderEffect;

std::vector<Sample> tone(double frequency, double amplitude, std::size_t frames)
{
    std::vector<Sample> samples(frames, 0.0f);
    for (std::size_t index = 0; index < frames; ++index)
        samples[index] = static_cast<Sample>(
            amplitude * std::sin(2.0 * pi * frequency * static_cast<double>(index) / sampleRate));

    return samples;
}

/// A carrier with energy everywhere the bank can find it.
std::vector<Sample> saw(double frequency, double amplitude, std::size_t frames)
{
    std::vector<Sample> samples(frames, 0.0f);

    double phase = 0.0;
    const double increment = frequency / sampleRate;

    for (std::size_t index = 0; index < frames; ++index) {
        samples[index] = static_cast<Sample>(amplitude * (2.0 * phase - 1.0));
        phase += increment;
        if (phase >= 1.0)
            phase -= 1.0;
    }

    return samples;
}

/// Runs the effect with a carrier on input 0 and a modulator on input 1,
/// which is the shape the graph compiler builds for a sidechain edge.
std::vector<Sample> processKeyed(Node& node, const std::vector<Sample>& carrier,
                                 const std::vector<Sample>& modulator, bool wireKey)
{
    node.prepare(sampleRate, blockSize);

    if (wireKey)
        if (auto* keyed = dynamic_cast<KeyedEffect*>(&node))
            keyed->setKeyInput(1);

    AudioBufferPool pool;
    pool.allocate(3, 1, blockSize);

    std::vector<Sample> output;
    output.reserve(carrier.size());

    const auto frames = static_cast<FrameCount>(carrier.size());

    for (FrameCount start = 0; start < frames; start += blockSize) {
        const FrameCount count = std::min<FrameCount>(blockSize, frames - start);

        const AudioBufferView carrierView   = pool.buffer(0);
        const AudioBufferView modulatorView = pool.buffer(1);
        const AudioBufferView outputView    = pool.buffer(2);

        carrierView.clear();
        modulatorView.clear();
        outputView.clear();

        for (FrameCount frame = 0; frame < count; ++frame) {
            const auto index = static_cast<std::size_t>(start + frame);
            carrierView.channel(0)[frame]   = carrier[index];
            modulatorView.channel(0)[frame] = modulator[index];
        }

        const AudioBufferView inputs[] = {carrierView.subBlock(0, count),
                                          modulatorView.subBlock(0, count)};

        ProcessContext context{};
        context.output     = outputView.subBlock(0, count);
        context.inputs     = inputs;
        context.inputCount = 2;
        context.frameCount = count;

        node.process(context);

        for (FrameCount frame = 0; frame < count; ++frame)
            output.push_back(outputView.channel(0)[frame]);
    }

    return output;
}

/// Energy in a band around `centre`, as a fraction of the whole.
double bandShare(const std::vector<Sample>& samples, double centre, double halfWidth)
{
    constexpr std::size_t length = 16384;

    std::vector<float> block(length, 0.0f);
    const std::size_t  from = samples.size() > length ? samples.size() - length : 0;

    for (std::size_t index = 0; index < length && from + index < samples.size(); ++index) {
        const double window = 0.5 - 0.5 * std::cos(2.0 * pi * static_cast<double>(index)
                                                   / static_cast<double>(length));
        block[index] = static_cast<float>(static_cast<double>(samples[from + index]) * window);
    }

    Fft fft;
    fft.setSize(length);

    std::vector<float> imaginary(length, 0.0f);
    fft.forward(block.data(), imaginary.data());

    double inside = 0.0;
    double total  = 0.0;

    for (std::size_t bin = 1; bin < length / 2; ++bin) {
        const double hz = static_cast<double>(bin) * sampleRate / static_cast<double>(length);
        const double magnitude = std::hypot(static_cast<double>(block[bin]),
                                            static_cast<double>(imaginary[bin]));

        const double energy = magnitude * magnitude;
        total += energy;

        if (std::fabs(hz - centre) < halfWidth)
            inside += energy;
    }

    return total > 0.0 ? inside / total : 0.0;
}

} // namespace

TEST_CASE("the vocoder is in the catalogue and takes a key")
{
    const BuiltinEffectInfo* info = findBuiltinEffect("incdaw.vocoder");
    REQUIRE(info != nullptr);

    CHECK(info->parameterCount == 8);
    CHECK(info->presets.count >= 3);

    std::unique_ptr<Node> node = makeBuiltinEffect("incdaw.vocoder", sampleRate);
    REQUIRE(node != nullptr);

    // Which effects accept a sidechain is now a question asked of the node,
    // not of a list of uids.
    CHECK(dynamic_cast<KeyedEffect*>(node.get()) != nullptr);
    CHECK(dynamic_cast<KeyedEffect*>(
              makeBuiltinEffect("incdaw.compressor", sampleRate).get()) != nullptr);
    CHECK(dynamic_cast<KeyedEffect*>(
              makeBuiltinEffect("incdaw.reverb", sampleRate).get()) == nullptr);
}

TEST_CASE("the bands are spread logarithmically across the voice")
{
    CHECK(VocoderEffect::bandCentreHz(0, 20) == doctest::Approx(VocoderEffect::lowestBandHz));
    CHECK(VocoderEffect::bandCentreHz(19, 20) == doctest::Approx(VocoderEffect::highestBandHz));

    // Equal ratios, not equal differences — the ear hears ratios, and equal
    // hertz would put most of the bank above the top of a voice.
    const double first  = VocoderEffect::bandCentreHz(1, 20)
                        / VocoderEffect::bandCentreHz(0, 20);
    const double last   = VocoderEffect::bandCentreHz(19, 20)
                        / VocoderEffect::bandCentreHz(18, 20);

    CHECK(first == doctest::Approx(last).epsilon(1e-6));
}

TEST_CASE("with no modulator wired in, the carrier passes through untouched")
{
    std::unique_ptr<Node> node = makeBuiltinEffect("incdaw.vocoder", sampleRate);
    node->parameterSink()->setParameter(Voc::mix, 1.0);

    const std::vector<Sample> carrier = saw(110.0, 0.5, 8192);
    const std::vector<Sample> silence(8192, 0.0f);

    const std::vector<Sample> output = processKeyed(*node, carrier, silence, false);

    for (std::size_t index = 0; index < carrier.size(); ++index)
        REQUIRE(output[index] == carrier[index]);
}

TEST_CASE("the output takes its shape from the modulator")
{
    std::unique_ptr<Node> node = makeBuiltinEffect("incdaw.vocoder", sampleRate);
    ParameterSink* sink = node->parameterSink();
    sink->setParameter(Voc::mix, 1.0);
    sink->setParameter(Voc::sibilance, 0.0);   // measure the bank alone
    sink->setParameter(Voc::resonance, 8.0);

    const std::vector<Sample> carrier = saw(110.0, 0.5, 32768);

    // A modulator with all of its energy near 1 kHz. The carrier has energy
    // everywhere; only the band the modulator excites should survive.
    const std::vector<Sample> modulator = tone(1000.0, 0.5, 32768);

    const std::vector<Sample> output = processKeyed(*node, carrier, modulator, true);

    const double before = bandShare(carrier, 1000.0, 250.0);
    const double after  = bandShare(output, 1000.0, 250.0);

    CAPTURE(before);
    CAPTURE(after);
    CHECK(after > before * 3.0);
    CHECK(after > 0.3);
}

TEST_CASE("a silent modulator gates the carrier off")
{
    std::unique_ptr<Node> node = makeBuiltinEffect("incdaw.vocoder", sampleRate);
    ParameterSink* sink = node->parameterSink();
    sink->setParameter(Voc::mix, 1.0);
    sink->setParameter(Voc::releaseMs, 1.0);

    const std::vector<Sample> carrier = saw(110.0, 0.5, 16384);
    const std::vector<Sample> silence(16384, 0.0f);

    const std::vector<Sample> output = processKeyed(*node, carrier, silence, true);

    double peak = 0.0;
    for (std::size_t index = 4096; index < output.size(); ++index)
        peak = std::max(peak, std::fabs(static_cast<double>(output[index])));

    CHECK(peak < 1e-4);
}

TEST_CASE("formant shifts the carrier's bands against the modulator's")
{
    const auto shareAtFormant = [](double semitones) {
        std::unique_ptr<Node> node = makeBuiltinEffect("incdaw.vocoder", sampleRate);
        ParameterSink* sink = node->parameterSink();
        sink->setParameter(Voc::mix, 1.0);
        sink->setParameter(Voc::sibilance, 0.0);
        sink->setParameter(Voc::resonance, 8.0);
        sink->setParameter(Voc::formant, semitones);

        const std::vector<Sample> output = processKeyed(*node, saw(110.0, 0.5, 32768),
                                                        tone(1000.0, 0.5, 32768), true);

        return bandShare(output, 2000.0, 400.0);
    };

    // An octave of formant moves the energy the 1 kHz modulator excites up to
    // about 2 kHz.
    CHECK(shareAtFormant(12.0) > shareAtFormant(0.0) * 2.0);
}

TEST_CASE("sibilance lets consonants through when the bank cannot")
{
    // The claim is specifically that the MODULATOR'S OWN top end appears at
    // the output — not that the output gets louder, which it need not.
    const auto shareAt13kWith = [](double sibilance) {
        std::unique_ptr<Node> node = makeBuiltinEffect("incdaw.vocoder", sampleRate);
        ParameterSink* sink = node->parameterSink();
        sink->setParameter(Voc::mix, 1.0);
        sink->setParameter(Voc::sibilance, sibilance);

        // A modulator that is only top end — an "s" with nothing under it,
        // and well above the highest band the bank has to offer.
        const std::vector<Sample> output =
            processKeyed(*node, saw(110.0, 0.5, 32768), tone(13000.0, 0.5, 32768), true);

        return bandShare(output, 13000.0, 400.0);
    };

    CHECK(shareAt13kWith(1.0) > shareAt13kWith(0.0) * 5.0);
}

TEST_CASE("the modulator never reaches the audio path")
{
    std::unique_ptr<Node> node = makeBuiltinEffect("incdaw.vocoder", sampleRate);
    ParameterSink* sink = node->parameterSink();
    sink->setParameter(Voc::mix, 1.0);
    sink->setParameter(Voc::sibilance, 0.0);

    // A silent carrier: whatever comes out can only have come from the
    // modulator leaking, and nothing should.
    const std::vector<Sample> silence(8192, 0.0f);
    const std::vector<Sample> modulator = tone(1000.0, 0.9, 8192);

    const std::vector<Sample> output = processKeyed(*node, silence, modulator, true);

    for (const Sample sample : output)
        REQUIRE(std::fabs(static_cast<double>(sample)) < 1e-6);
}
