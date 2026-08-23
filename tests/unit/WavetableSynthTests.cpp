// The wavetable synth (A1).
//
// The tests that matter here are the ones a listener would notice and a
// waveform plot would not: is the note in tune, and does the top of the
// keyboard stay clean? Both are spectral, so both are measured with an FFT of
// the synth's actual output rather than by inspecting its internals.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/Fft.h"
#include "engine/instrument/BuiltinInstruments.h"
#include "engine/instrument/WavetableSynth.h"

#include <cmath>
#include <numbers>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace rt = incdaw::engine::rt;

namespace {

constexpr double rate = 48000.0;

/// Renders `frames` of one held note into a flat mono buffer.
std::vector<float> renderNote(WavetableSynth& synth, int key, std::size_t frames,
                              std::size_t skip = 0)
{
    AudioBufferPool pool;
    pool.allocate(1, 1, 512);

    MidiBuffer start;
    start.insert(MidiMessage::noteOn(0, key, 100, 0));

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
    // Hann window: the note is not periodic in the analysis length, and a
    // rectangular window would smear every harmonic across the spectrum.
    const std::size_t length = samples.size();
    for (std::size_t index = 0; index < length; ++index) {
        const double window =
            0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(index)
                                 / static_cast<double>(length));
        samples[index] = static_cast<float>(static_cast<double>(samples[index]) * window);
    }

    dsp::Fft fft;
    fft.setSize(length);

    std::vector<float> imaginary(length, 0.0f);
    fft.forward(samples.data(), imaginary.data());

    std::vector<double> result(length / 2 + 1, 0.0);
    for (std::size_t bin = 0; bin < result.size(); ++bin)
        result[bin] = std::hypot(static_cast<double>(samples[bin]),
                                 static_cast<double>(imaginary[bin]));

    return result;
}

void set(WavetableSynth& synth, WavetableParam parameter, double value)
{
    synth.setParameter(static_cast<std::uint32_t>(parameter), value);
}

double get(const WavetableSynth& synth, WavetableParam parameter)
{
    return synth.value(static_cast<std::uint32_t>(parameter));
}

} // namespace

TEST_CASE("the synth is in the instrument catalogue with its parameters and presets")
{
    const BuiltinInstrumentInfo* info = findBuiltinInstrument("incdaw.wavetable");
    REQUIRE(info != nullptr);

    CHECK(info->parameterCount == wavetableParameterCount());
    CHECK(info->parameterCount == WavetableSynth::parameterCount);
    CHECK(info->presets.count >= 4);
}

TEST_CASE("a fresh synth holds its declared defaults")
{
    WavetableSynth synth;

    for (std::size_t index = 0; index < wavetableParameterCount(); ++index) {
        const dsp::EffectParameter& parameter = wavetableParameters()[index];
        CAPTURE(parameter.name);
        CHECK(synth.value(parameter.id) == doctest::Approx(parameter.defaultValue));
    }
}

TEST_CASE("parameters clamp into range and stepped ones land on integers")
{
    WavetableSynth synth;

    set(synth, WavetableParam::oscALevel, 5.0);
    CHECK(get(synth, WavetableParam::oscALevel) == doctest::Approx(1.0));

    set(synth, WavetableParam::oscADetune, -900.0);
    CHECK(get(synth, WavetableParam::oscADetune) == doctest::Approx(-50.0));

    set(synth, WavetableParam::oscATable, 2.4);
    CHECK(get(synth, WavetableParam::oscATable) == doctest::Approx(2.0));

    // An id the synth does not have is ignored rather than misfiled.
    synth.setParameter(9999u, 1.0);
    CHECK(synth.value(9999u) == doctest::Approx(0.0));
}

TEST_CASE("a held note is in tune")
{
    WavetableSynth synth;
    synth.prepare(rate, 512);

    // A4 = 440 Hz, and no detune so the measurement is of the oscillator.
    set(synth, WavetableParam::oscADetune, 0.0);
    set(synth, WavetableParam::filterMode, 0.0);

    constexpr std::size_t frames = 16384;
    const std::vector<double> spectrum = magnitudes(renderNote(synth, 69, frames, 2048));

    std::size_t peak = 1;
    for (std::size_t bin = 1; bin < spectrum.size(); ++bin)
        if (spectrum[bin] > spectrum[peak])
            peak = bin;

    const double hz = static_cast<double>(peak) * rate / static_cast<double>(frames);
    const double cents = 1200.0 * std::log2(hz / 440.0);

    CAPTURE(hz);
    CHECK(std::abs(cents) < 6.0);   // one bin is ~2.9 Hz, about 11 cents wide
}

TEST_CASE("the top of the keyboard does not alias")
{
    WavetableSynth synth;
    synth.prepare(rate, 512);

    // A bright sawtooth, no filter to hide behind, near the top of a piano.
    set(synth, WavetableParam::oscAPosition, 2.0 / 3.0);
    set(synth, WavetableParam::oscADetune, 0.0);
    set(synth, WavetableParam::oscALevel, 1.0);
    set(synth, WavetableParam::filterMode, 0.0);
    set(synth, WavetableParam::ampAttack, 0.0);
    set(synth, WavetableParam::ampSustain, 1.0);

    constexpr int         key    = 105;   // ~3520 Hz
    constexpr std::size_t frames = 16384;

    const double frequency = 440.0 * std::pow(2.0, (key - 69) / 12.0);
    const std::vector<double> spectrum = magnitudes(renderNote(synth, key, frames, 2048));

    const double binHz = rate / static_cast<double>(frames);

    double harmonic = 0.0;
    double alias    = 0.0;

    for (std::size_t bin = 4; bin < spectrum.size(); ++bin) {
        const double hz    = static_cast<double>(bin) * binHz;
        const double ratio = hz / frequency;
        const double near  = std::round(ratio);

        // A Hann window's skirt is what is being excluded here, not the
        // harmonic: 3520 Hz does not land on a bin, so each harmonic leaks
        // into its neighbours at around -40 dB three bins out and -70 dB
        // eight bins out. Anything closer than that measures the window
        // rather than the synth.
        if (near >= 1.0 && std::abs(hz - near * frequency) < binHz * 8.0) {
            harmonic = std::max(harmonic, spectrum[bin]);
            continue;
        }

        alias = std::max(alias, spectrum[bin]);
    }

    CAPTURE(harmonic);
    CAPTURE(alias);
    CHECK(alias < harmonic * 1e-3);   // 60 dB below the note
}

TEST_CASE("a note starts, sustains and ends")
{
    WavetableSynth synth;
    synth.prepare(rate, 512);

    AudioBufferPool pool;
    pool.allocate(1, 1, 512);

    CHECK(synth.activeVoiceCount() == 0);

    MidiBuffer on;
    on.insert(MidiMessage::noteOn(0, 60, 100, 0));

    auto output = pool.buffer(0);
    output.clear();
    synth.processBlock(output, on);

    CHECK(synth.activeVoiceCount() == 1);
    CHECK(output.peak() > 0.0f);

    MidiBuffer off;
    off.insert(MidiMessage::noteOff(0, 60, 0, 0));

    set(synth, WavetableParam::ampRelease, 0.001);
    output.clear();
    synth.processBlock(output, off);

    // 512 frames is ten releases at a millisecond.
    output.clear();
    synth.processBlock(output, MidiBuffer{});
    CHECK(synth.activeVoiceCount() == 0);
}

TEST_CASE("all notes off silences everything, from the message and from the call")
{
    WavetableSynth synth;
    synth.prepare(rate, 512);

    AudioBufferPool pool;
    pool.allocate(1, 1, 512);

    MidiBuffer chord;
    for (int key = 60; key < 66; ++key)
        chord.insert(MidiMessage::noteOn(0, key, 100, 0));

    auto output = pool.buffer(0);
    output.clear();
    synth.processBlock(output, chord);
    CHECK(synth.activeVoiceCount() == 6);

    MidiBuffer panic;
    panic.insert(MidiMessage::controlChange(0, 123, 0, 0));

    output.clear();
    synth.processBlock(output, panic);
    CHECK(synth.activeVoiceCount() == 0);

    output.clear();
    synth.processBlock(output, chord);
    CHECK(synth.activeVoiceCount() == 6);

    synth.allNotesOff();
    CHECK(synth.activeVoiceCount() == 0);
}

TEST_CASE("more notes than voices steals rather than dropping or stacking")
{
    WavetableSynth synth;
    synth.prepare(rate, 512);

    AudioBufferPool pool;
    pool.allocate(1, 1, 512);

    MidiBuffer many;
    for (int key = 24; key < 24 + WavetableSynth::maxVoices + 8; ++key)
        many.insert(MidiMessage::noteOn(0, key, 100, 0));

    const auto output = pool.buffer(0);
    output.clear();
    synth.processBlock(output, many);

    CHECK(synth.activeVoiceCount() == WavetableSynth::maxVoices);
    CHECK(std::isfinite(static_cast<double>(output.peak())));
}

TEST_CASE("retriggering a held key does not stack two voices on it")
{
    WavetableSynth synth;
    synth.prepare(rate, 512);

    AudioBufferPool pool;
    pool.allocate(1, 1, 512);

    MidiBuffer twice;
    twice.insert(MidiMessage::noteOn(0, 60, 100, 0));
    twice.insert(MidiMessage::noteOn(0, 60, 100, 128));

    const auto output = pool.buffer(0);
    output.clear();
    synth.processBlock(output, twice);

    // One sounding, one released — never two at full level on one key.
    CHECK(synth.activeVoiceCount() <= 2);
}

TEST_CASE("every modulation destination changes the sound, and none of it blows up")
{
    const auto renderWith = [](WavetableParam parameter, double value) {
        WavetableSynth synth;
        synth.prepare(rate, 512);
        set(synth, WavetableParam::oscALevel, 0.8);
        set(synth, parameter, value);

        const std::vector<float> rendered = renderNote(synth, 60, 8192, 512);

        double energy = 0.0;
        for (const float sample : rendered) {
            REQUIRE(std::isfinite(sample));
            energy += static_cast<double>(sample) * static_cast<double>(sample);
        }

        return energy;
    };

    const double plain = renderWith(WavetableParam::gain, 0.5);

    CHECK(renderWith(WavetableParam::lfo1ToPitch, 7.0) != doctest::Approx(plain));
    CHECK(renderWith(WavetableParam::lfo1ToPosition, 0.8) != doctest::Approx(plain));
    CHECK(renderWith(WavetableParam::lfo1ToCutoff, 4.0) != doctest::Approx(plain));
    CHECK(renderWith(WavetableParam::modToPitch, 12.0) != doctest::Approx(plain));
    CHECK(renderWith(WavetableParam::modToPosition, 1.0) != doctest::Approx(plain));
    // Downward: the default cutoff is already above anything the note
    // contains, so opening it further would change nothing.
    CHECK(renderWith(WavetableParam::modToCutoff, -6.0) != doctest::Approx(plain));
    CHECK(renderWith(WavetableParam::subLevel, 0.7) != doctest::Approx(plain));
    CHECK(renderWith(WavetableParam::oscBLevel, 0.7) != doctest::Approx(plain));
}

TEST_CASE("rendering allocates nothing on the audio thread")
{
    WavetableSynth synth;
    synth.prepare(rate, 512);

    AudioBufferPool pool;
    pool.allocate(1, 2, 512);

    MidiBuffer chord;
    for (int key = 48; key < 60; ++key)
        chord.insert(MidiMessage::noteOn(0, key, 100, 0));

    const auto output = pool.buffer(0);
    output.clear();
    synth.processBlock(output, chord);

    rt::resetViolations();

    {
        const rt::ScopedRealtimeContext scope;
        for (int block = 0; block < 8; ++block) {
            output.clear();
            synth.processBlock(output, MidiBuffer{});
            synth.setParameter(static_cast<std::uint32_t>(WavetableParam::oscAPosition),
                               0.1 * static_cast<double>(block));
        }
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(std::isfinite(static_cast<double>(output.peak())));
}
