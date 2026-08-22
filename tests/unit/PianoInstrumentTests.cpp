#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/instrument/BuiltinInstruments.h"
#include "engine/instrument/PianoInstrument.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

constexpr SampleRate rate   = 48000.0;
constexpr FrameCount block  = 512;

struct Rendered {
    double peak = 0.0;
    double rms  = 0.0;
    bool   finite = true;
};

/// Renders `blocks` blocks, optionally delivering `midi` into the first one,
/// and answers what came out. Every test here asks one of these three things.
Rendered render(PianoInstrument& piano, AudioBufferPool& pool, const MidiBuffer& midi,
                int blocks)
{
    Rendered result;
    MidiBuffer empty;

    for (int index = 0; index < blocks; ++index) {
        const auto output = pool.buffer(0).subBlock(0, block);
        output.clear();

        {
            // The render path is asserted realtime-safe here rather than in a
            // separate test: every block every test renders is covered.
            rt::ScopedRealtimeContext realtime;
            piano.processBlock(output, index == 0 ? midi : empty);
        }

        for (std::size_t channel = 0; channel < output.channelCount(); ++channel) {
            const Sample* samples = output.channel(channel);
            for (FrameCount frame = 0; frame < block; ++frame) {
                const double value = static_cast<double>(samples[frame]);

                if (!std::isfinite(value))
                    result.finite = false;

                result.peak = std::max(result.peak, std::abs(value));
                result.rms += value * value;
            }
        }
    }

    result.rms = std::sqrt(result.rms / static_cast<double>(blocks * block * 2));
    return result;
}

MidiBuffer noteOn(int key, int velocity)
{
    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, key, velocity));
    return midi;
}

/// An instrument is neither copyable nor movable by design, so the fixture
/// prepares one in place rather than handing one back.
void makeReady(PianoInstrument& piano, AudioBufferPool& pool)
{
    pool.allocate(1, 2, block);
    piano.prepare(rate, block);
}

} // namespace

TEST_CASE("piano sounds and then stops")
{
    AudioBufferPool pool;
    pool.allocate(1, 2, block);

    PianoInstrument piano;
    piano.prepare(rate, block);

    CHECK(piano.activeVoiceCount() == 0);

    const Rendered struck = render(piano, pool, noteOn(60, 100), 4);

    CHECK(piano.activeVoiceCount() == 1);
    CHECK(struck.finite);
    CHECK(struck.peak > 0.01);

    // A note-off damps but does not cut: the string is still ringing in the
    // block immediately after the key comes up.
    MidiBuffer off;
    off.insert(MidiMessage::noteOff(0, 60));
    const Rendered damping = render(piano, pool, off, 2);
    CHECK(damping.peak > 0.0);

    // …and it is gone within a second, which is what the damper time means.
    MidiBuffer empty;
    (void)render(piano, pool, empty, 100);
    CHECK(piano.activeVoiceCount() == 0);

    // Measured AFTER the damper has run, not across it: the peak of the whole
    // fall is the peak of its first sample.
    const Rendered silent = render(piano, pool, empty, 1);
    CHECK(silent.peak < 1.0e-4);
}

TEST_CASE("the sustain pedal holds the note through its key")
{
    AudioBufferPool pool;
    PianoInstrument piano;
    makeReady(piano, pool);

    MidiBuffer start;
    start.insert(MidiMessage::controlChange(0, 64, 127));
    start.insert(MidiMessage::noteOn(0, 55, 110));
    (void)render(piano, pool, start, 2);

    CHECK(piano.sustainPedalDown());
    CHECK(piano.activeVoiceCount() == 1);

    MidiBuffer keyUp;
    keyUp.insert(MidiMessage::noteOff(0, 55));
    (void)render(piano, pool, keyUp, 40);

    // Still ringing half a second after the key came up, because the damper is
    // off the string.
    CHECK(piano.activeVoiceCount() == 1);

    MidiBuffer pedalUp;
    pedalUp.insert(MidiMessage::controlChange(0, 64, 0));
    (void)render(piano, pool, pedalUp, 120);

    CHECK_FALSE(piano.sustainPedalDown());
    CHECK(piano.activeVoiceCount() == 0);
}

TEST_CASE("all notes off silences everything, pedal included")
{
    AudioBufferPool pool;
    PianoInstrument piano;
    makeReady(piano, pool);

    MidiBuffer chord;
    chord.insert(MidiMessage::controlChange(0, 64, 127));
    chord.insert(MidiMessage::noteOn(0, 60, 100));
    chord.insert(MidiMessage::noteOn(0, 64, 100));
    chord.insert(MidiMessage::noteOn(0, 67, 100));
    (void)render(piano, pool, chord, 2);
    CHECK(piano.activeVoiceCount() == 3);

    MidiBuffer panic;
    panic.insert(MidiMessage::controlChange(0, 123, 0));
    const Rendered after = render(piano, pool, panic, 1);

    CHECK(piano.activeVoiceCount() == 0);
    CHECK_FALSE(piano.sustainPedalDown());
    CHECK(after.peak < 1.0e-6);
}

TEST_CASE("a re-struck key is one string, not two")
{
    AudioBufferPool pool;
    PianoInstrument piano;
    makeReady(piano, pool);

    (void)render(piano, pool, noteOn(60, 100), 1);
    (void)render(piano, pool, noteOn(60, 100), 1);

    // The first voice is damped rather than left to sum with the second, so
    // the level cannot double. Both are still allocated while the old one
    // fades; what matters is that it is on its way out.
    (void)render(piano, pool, MidiBuffer{}, 20);
    CHECK(piano.activeVoiceCount() == 1);
}

TEST_CASE("every model plays, and they do not sound the same")
{
    AudioBufferPool pool;
    std::vector<double> rms;

    for (int model = 0; model < pianoModelCount; ++model) {
        PianoInstrument piano;
        makeReady(piano, pool);
        piano.setParameter(static_cast<std::uint32_t>(PianoParam::model),
                           static_cast<double>(model));

        const Rendered played = render(piano, pool, noteOn(60, 100), 8);

        CAPTURE(model);
        CHECK(played.finite);
        CHECK(played.peak > 0.01);
        CHECK(played.peak < 1.5);   // normalisation keeps a single note in range
        rms.push_back(played.rms);
    }

    REQUIRE(rms.size() == static_cast<std::size_t>(pianoModelCount));

    // Models change the physics, not just the level: no two of them may render
    // an identical block. This is what stops a "model" from becoming a label.
    for (std::size_t a = 0; a + 1 < rms.size(); ++a)
        for (std::size_t b = a + 1; b < rms.size(); ++b)
            CHECK(std::abs(rms[a] - rms[b]) > 1.0e-6);
}

TEST_CASE("the whole keyboard is finite and in range")
{
    AudioBufferPool pool;

    for (int key = 21; key <= 108; key += 3) {
        PianoInstrument piano;
        makeReady(piano, pool);
        const Rendered played = render(piano, pool, noteOn(key, 127), 4);

        CAPTURE(key);
        CHECK(played.finite);
        CHECK(played.peak > 0.001);
        CHECK(played.peak < 1.5);
    }
}

TEST_CASE("a full keyboard of voices stays bounded and steals cleanly")
{
    AudioBufferPool pool;
    PianoInstrument piano;
    makeReady(piano, pool);

    MidiBuffer pedal;
    pedal.insert(MidiMessage::controlChange(0, 64, 127));
    (void)render(piano, pool, pedal, 1);

    // More notes than there are voices, with the pedal down so none of them
    // retire: the allocator has to steal, and nothing may go non-finite.
    for (int key = 21; key <= 108; ++key) {
        const Rendered played = render(piano, pool, noteOn(key, 96), 1);
        CAPTURE(key);
        CHECK(played.finite);
    }

    CHECK(piano.activeVoiceCount() <= PianoInstrument::maxVoices);
    CHECK(piano.activeVoiceCount() > 0);
}

TEST_CASE("the same events render the same audio twice")
{
    AudioBufferPool poolA;
    AudioBufferPool poolB;

    PianoInstrument first;
    PianoInstrument second;
    makeReady(first, poolA);
    makeReady(second, poolB);

    const Rendered a = render(first, poolA, noteOn(64, 88), 6);
    const Rendered b = render(second, poolB, noteOn(64, 88), 6);

    // Offline render equivalence rests on this: the hammer noise is seeded
    // from the strike, not from a clock (CLAUDE.md §29).
    CHECK(a.peak == b.peak);
    CHECK(a.rms == b.rms);
}

TEST_CASE("the piano is in the instrument catalogue with its parameters")
{
    const BuiltinInstrumentInfo* info = findBuiltinInstrument("incdaw.piano");
    REQUIRE(info != nullptr);
    CHECK(std::string{info->displayName} == "INCDAW Piano");
    CHECK(info->parameterCount == 10);

    // Every catalogue parameter must reach a real setter, or the panel would
    // show a control that does nothing.
    AudioBufferPool pool;
    PianoInstrument piano;
    makeReady(piano, pool);

    for (std::size_t index = 0; index < info->parameterCount; ++index) {
        const dsp::EffectParameter& parameter = info->parameters[index];
        piano.setParameter(parameter.id, parameter.maxValue);
        piano.setParameter(parameter.id, parameter.minValue);
        piano.setParameter(parameter.id, parameter.defaultValue);
    }

    const Rendered played = render(piano, pool, noteOn(72, 64), 4);
    CHECK(played.finite);
}

TEST_CASE("model names cover the catalogue and never return null")
{
    for (int model = -2; model < pianoModelCount + 2; ++model)
        CHECK(pianoModelName(model) != nullptr);

    CHECK(std::string{pianoModelName(0)} == "Grand");
    CHECK(std::string{pianoModelName(4)} == "Electric");
}
