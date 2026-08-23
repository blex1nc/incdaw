// The FM synth (A2).
//
// FM's correctness has a closed form, which is unusual and worth using: a
// sine carrier phase-modulated by a sine at index I has sidebands whose
// amplitudes are the Bessel functions J_k(I). The test below computes those
// Bessels by numerical integration — a definition with nothing in common with
// the synth's code — and holds the rendered spectrum to them.
//
// That is a much stronger statement than "it makes a sound": it says the
// modulation depth means what it claims, the operator routing goes where it
// says, and the one-sample delay that makes cyclic matrices computable has
// not quietly changed the maths.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/Fft.h"
#include "engine/instrument/BuiltinInstruments.h"
#include "engine/instrument/FmSynth.h"

#include <cmath>
#include <numbers>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace rt = incdaw::engine::rt;

namespace {

constexpr double rate = 48000.0;
constexpr double pi   = std::numbers::pi;

/// J_k(x) by the integral definition, trapezoid rule. Independent of
/// everything the synth does — that is the point.
double besselJ(int order, double argument)
{
    constexpr int steps = 200000;

    double sum = 0.0;
    for (int step = 0; step < steps; ++step) {
        const double theta = pi * (static_cast<double>(step) + 0.5)
                           / static_cast<double>(steps);
        sum += std::cos(static_cast<double>(order) * theta - argument * std::sin(theta));
    }

    return sum / static_cast<double>(steps);
}

std::vector<float> renderNote(FmSynth& synth, int key, std::size_t frames, std::size_t skip)
{
    AudioBufferPool pool;
    pool.allocate(1, 1, 512);

    MidiBuffer start;
    start.insert(MidiMessage::noteOn(0, key, 127, 0));

    std::vector<float> rendered;
    rendered.reserve(frames);

    std::size_t produced = 0;
    bool        first    = true;

    while (produced < frames + skip) {
        const auto output = pool.buffer(0);
        output.clear();
        synth.processBlock(output, first ? start : MidiBuffer{});
        first = false;

        const float* samples = output.channel(0);
        for (FrameCount frame = 0; frame < 512 && produced < frames + skip; ++frame, ++produced)
            if (produced >= skip)
                rendered.push_back(samples[frame]);
    }

    return rendered;
}

std::vector<double> magnitudes(std::vector<float> samples)
{
    dsp::Fft fft;
    fft.setSize(samples.size());

    std::vector<float> imaginary(samples.size(), 0.0f);
    fft.forward(samples.data(), imaginary.data());

    std::vector<double> result(samples.size() / 2 + 1, 0.0);
    for (std::size_t bin = 0; bin < result.size(); ++bin)
        result[bin] = std::hypot(static_cast<double>(samples[bin]),
                                 static_cast<double>(imaginary[bin]));

    return result;
}

void set(FmSynth& synth, std::uint32_t parameterId, double value)
{
    synth.setParameter(parameterId, value);
}

/// A carrier and one modulator, both at fixed pitches that land exactly on
/// FFT bins, with flat envelopes so the spectrum is stationary.
void twoOperatorPatch(FmSynth& synth, double carrierHz, double modulatorHz, double amount)
{
    for (int index = 0; index < FmSynth::operatorCount; ++index) {
        set(synth, FmParam::forOperator(index, FmParam::outLevel), 0.0);
        set(synth, FmParam::forOperator(index, FmParam::attack), 0.0);
        set(synth, FmParam::forOperator(index, FmParam::decay), 0.0);
        set(synth, FmParam::forOperator(index, FmParam::sustain), 1.0);

        for (int destination = 0; destination < FmSynth::operatorCount; ++destination)
            set(synth, FmParam::forRoute(index, destination), 0.0);
    }

    set(synth, FmParam::forOperator(0, FmParam::fixedHz), carrierHz);
    set(synth, FmParam::forOperator(0, FmParam::outLevel), 1.0);
    set(synth, FmParam::forOperator(1, FmParam::fixedHz), modulatorHz);
    set(synth, FmParam::forRoute(1, 0), amount);
}

} // namespace

TEST_CASE("the synth is in the catalogue with its matrix and its presets")
{
    const BuiltinInstrumentInfo* info = findBuiltinInstrument("incdaw.fm");
    REQUIRE(info != nullptr);

    CHECK(info->parameterCount == FmSynth::parameterCount);
    CHECK(info->parameterCount == 85);
    CHECK(info->presets.count >= 4);

    // Every route in the matrix is an ordinary parameter, which is the whole
    // claim of the design.
    for (int source = 0; source < FmSynth::operatorCount; ++source)
        for (int destination = 0; destination < FmSynth::operatorCount; ++destination) {
            const std::uint32_t id = FmParam::forRoute(source, destination);

            bool found = false;
            for (std::size_t index = 0; index < info->parameterCount; ++index)
                found = found || info->parameters[index].id == id;

            CAPTURE(source);
            CAPTURE(destination);
            CHECK(found);
        }
}

TEST_CASE("a fresh synth holds its declared defaults")
{
    FmSynth synth;

    for (std::size_t index = 0; index < fmParameterCount(); ++index) {
        const dsp::EffectParameter& parameter = fmParameters()[index];
        CAPTURE(parameter.name);
        CHECK(synth.value(parameter.id) == doctest::Approx(parameter.defaultValue));
    }
}

TEST_CASE("the sidebands are the Bessel functions of the modulation index")
{
    constexpr std::size_t frames = 16384;

    const double binHz      = rate / static_cast<double>(frames);
    const double carrierHz  = 683.0 * binHz;   // ~2001 Hz, exactly on a bin
    const double modulatorHz = 68.0 * binHz;   // ~199 Hz, exactly on a bin

    // The modulator's output is added to the carrier's phase in CYCLES, so
    // the index in radians is 2*pi times the route's amount.
    constexpr double amount = 0.25;
    const double     index  = 2.0 * pi * amount;

    FmSynth synth;
    synth.prepare(rate, 512);
    twoOperatorPatch(synth, carrierHz, modulatorHz, amount);

    const std::vector<double> spectrum = magnitudes(renderNote(synth, 69, frames, 4096));

    const double carrierBin = spectrum[683];
    REQUIRE(carrierBin > 0.0);

    for (int order = 0; order <= 4; ++order) {
        const double expected = std::abs(besselJ(order, index) / besselJ(0, index));

        const double upper = spectrum[static_cast<std::size_t>(683 + 68 * order)] / carrierBin;
        const double lower = spectrum[static_cast<std::size_t>(683 - 68 * order)] / carrierBin;

        CAPTURE(order);
        CAPTURE(expected);
        CAPTURE(upper);
        CAPTURE(lower);

        CHECK(upper == doctest::Approx(expected).epsilon(0.03));
        CHECK(lower == doctest::Approx(expected).epsilon(0.03));
    }
}

TEST_CASE("a deeper route makes more sidebands, exactly as the index says")
{
    constexpr std::size_t frames = 16384;

    const double binHz       = rate / static_cast<double>(frames);
    const double carrierHz   = 683.0 * binHz;
    const double modulatorHz = 68.0 * binHz;

    const auto sidebandAt = [&](double amount, int order) {
        FmSynth synth;
        synth.prepare(rate, 512);
        twoOperatorPatch(synth, carrierHz, modulatorHz, amount);

        const std::vector<double> spectrum = magnitudes(renderNote(synth, 69, frames, 4096));
        return spectrum[static_cast<std::size_t>(683 + 68 * order)];
    };

    // The fourth sideband is essentially absent at a small index and clearly
    // present at a large one.
    CHECK(sidebandAt(0.6, 4) > sidebandAt(0.1, 4) * 10.0);
}

TEST_CASE("feedback is the diagonal of the matrix, and it changes the sound")
{
    const auto energyWithFeedback = [](double amount) {
        FmSynth synth;
        synth.prepare(rate, 512);

        for (int index = 0; index < FmSynth::operatorCount; ++index)
            set(synth, FmParam::forOperator(index, FmParam::outLevel), index == 0 ? 1.0 : 0.0);

        set(synth, FmParam::forRoute(0, 0), amount);

        const std::vector<float> rendered = renderNote(synth, 60, 8192, 1024);

        double energy = 0.0;
        for (const float sample : rendered) {
            REQUIRE(std::isfinite(sample));
            energy += static_cast<double>(sample) * static_cast<double>(sample);
        }

        return energy;
    };

    const double clean = energyWithFeedback(0.0);
    const double fed   = energyWithFeedback(2.0);

    CHECK(fed != doctest::Approx(clean));
    CHECK(std::isfinite(fed));
}

TEST_CASE("a chain through three operators reaches the output")
{
    // 3 modulates 2 modulates 1: the sound must differ from the same patch
    // with the middle route cut, or the matrix is not being walked.
    const auto energyWith = [](double middleRoute) {
        FmSynth synth;
        synth.prepare(rate, 512);

        for (int index = 0; index < FmSynth::operatorCount; ++index)
            set(synth, FmParam::forOperator(index, FmParam::outLevel), index == 0 ? 1.0 : 0.0);

        set(synth, FmParam::forOperator(1, FmParam::ratio), 2.0);
        set(synth, FmParam::forOperator(2, FmParam::ratio), 3.0);
        set(synth, FmParam::forRoute(1, 0), 1.5);
        set(synth, FmParam::forRoute(2, 1), middleRoute);

        const std::vector<float> rendered = renderNote(synth, 60, 8192, 1024);

        double energy = 0.0;
        for (const float sample : rendered)
            energy += static_cast<double>(sample) * static_cast<double>(sample);

        return energy;
    };

    CHECK(energyWith(3.0) != doctest::Approx(energyWith(0.0)));
}

TEST_CASE("a fixed-frequency operator ignores the key")
{
    const auto peakBin = [](int key) {
        FmSynth synth;
        synth.prepare(rate, 512);
        twoOperatorPatch(synth, 683.0 * rate / 16384.0, 68.0 * rate / 16384.0, 0.0);

        const std::vector<double> spectrum = magnitudes(renderNote(synth, key, 16384, 4096));

        std::size_t peak = 1;
        for (std::size_t bin = 1; bin < spectrum.size(); ++bin)
            if (spectrum[bin] > spectrum[peak])
                peak = bin;

        return peak;
    };

    CHECK(peakBin(48) == peakBin(84));
    CHECK(peakBin(48) == 683);
}

TEST_CASE("notes start, stop, panic and steal")
{
    FmSynth synth;
    synth.prepare(rate, 512);

    AudioBufferPool pool;
    pool.allocate(1, 1, 512);

    const auto output = pool.buffer(0);

    MidiBuffer many;
    for (int key = 36; key < 36 + FmSynth::maxVoices + 6; ++key)
        many.insert(MidiMessage::noteOn(0, key, 100, 0));

    output.clear();
    synth.processBlock(output, many);

    CHECK(synth.activeVoiceCount() == FmSynth::maxVoices);
    CHECK(output.peak() > 0.0f);
    CHECK(std::isfinite(static_cast<double>(output.peak())));

    MidiBuffer panic;
    panic.insert(MidiMessage::controlChange(0, 120, 0, 0));

    output.clear();
    synth.processBlock(output, panic);
    CHECK(synth.activeVoiceCount() == 0);
}

TEST_CASE("a released note ends")
{
    FmSynth synth;
    synth.prepare(rate, 512);

    for (int index = 0; index < FmSynth::operatorCount; ++index)
        set(synth, FmParam::forOperator(index, FmParam::release), 0.001);

    AudioBufferPool pool;
    pool.allocate(1, 1, 512);
    const auto output = pool.buffer(0);

    MidiBuffer on;
    on.insert(MidiMessage::noteOn(0, 60, 100, 0));
    output.clear();
    synth.processBlock(output, on);
    CHECK(synth.activeVoiceCount() == 1);

    MidiBuffer off;
    off.insert(MidiMessage::noteOff(0, 60, 0, 0));
    output.clear();
    synth.processBlock(output, off);

    output.clear();
    synth.processBlock(output, MidiBuffer{});
    CHECK(synth.activeVoiceCount() == 0);
}

TEST_CASE("rendering allocates nothing on the audio thread")
{
    FmSynth synth;
    synth.prepare(rate, 512);

    AudioBufferPool pool;
    pool.allocate(1, 2, 512);
    const auto output = pool.buffer(0);

    MidiBuffer chord;
    for (int key = 48; key < 58; ++key)
        chord.insert(MidiMessage::noteOn(0, key, 100, 0));

    output.clear();
    synth.processBlock(output, chord);

    rt::resetViolations();

    {
        const rt::ScopedRealtimeContext scope;
        for (int block = 0; block < 8; ++block) {
            output.clear();
            synth.processBlock(output, MidiBuffer{});
            synth.setParameter(FmParam::forRoute(1, 0), 0.25 * static_cast<double>(block));
        }
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(std::isfinite(static_cast<double>(output.peak())));
}
