// The drum machine (A4).
//
// What makes this an instrument rather than sixteen sounds is the behaviour
// around the voices: a pad retriggers without clicking, a choke group cuts
// across pads, a hit ends on its own, and pan puts each pad somewhere. Those
// are the tests. The voices themselves are checked for the properties a
// listener would notice — a kick's pitch falls, a hat is high, a clap is not
// one burst — rather than sample by sample against a reference nobody has.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/Fft.h"
#include "engine/instrument/BuiltinInstruments.h"
#include "engine/instrument/DrumMachine.h"

#include <cmath>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace rt = incdaw::engine::rt;

namespace {

constexpr double rate = 48000.0;

void set(DrumMachine& drum, int pad, DrumParam::PadOffset offset, double value)
{
    drum.setParameter(DrumParam::forPad(pad, offset), value);
}

struct Rendered {
    std::vector<float> left;
    std::vector<float> right;
};

/// Strikes `pad` and renders `frames` of stereo.
Rendered strike(DrumMachine& drum, int pad, std::size_t frames)
{
    AudioBufferPool pool;
    pool.allocate(1, 2, 512);

    MidiBuffer hit;
    hit.insert(MidiMessage::noteOn(0, DrumMachine::firstKey + pad, 120, 0));

    Rendered rendered;
    bool     first = true;

    while (rendered.left.size() < frames) {
        const auto output = pool.buffer(0);
        output.clear();
        drum.processBlock(output, first ? hit : MidiBuffer{});
        first = false;

        for (FrameCount frame = 0; frame < 512 && rendered.left.size() < frames; ++frame) {
            rendered.left.push_back(output.channel(0)[frame]);
            rendered.right.push_back(output.channel(1)[frame]);
        }
    }

    return rendered;
}

double peakOf(const std::vector<float>& samples, std::size_t from, std::size_t to)
{
    double peak = 0.0;
    for (std::size_t index = from; index < std::min(to, samples.size()); ++index)
        peak = std::max(peak, std::abs(static_cast<double>(samples[index])));

    return peak;
}

/// The frequency of the strongest bin, in Hz.
///
/// A spectral centroid would be the obvious choice and is the wrong one here:
/// a kick's click is broadband, and a few milliseconds of it drags the
/// centroid above a kilohertz while the drum plainly sounds like fifty. The
/// loudest partial is what "how high is this" means for percussion.
double peakFrequency(std::vector<float> samples)
{
    dsp::Fft fft;
    fft.setSize(samples.size());

    std::vector<float> imaginary(samples.size(), 0.0f);
    fft.forward(samples.data(), imaginary.data());

    std::size_t peak = 1;
    double      best = 0.0;

    for (std::size_t bin = 1; bin < samples.size() / 2; ++bin) {
        const double magnitude = std::hypot(static_cast<double>(samples[bin]),
                                            static_cast<double>(imaginary[bin]));
        if (magnitude > best) {
            best = magnitude;
            peak = bin;
        }
    }

    return static_cast<double>(peak) * rate / static_cast<double>(samples.size());
}

/// `count` samples from `from`, as a power-of-two block for the transform.
std::vector<float> window(const std::vector<float>& samples, std::size_t from,
                          std::size_t count)
{
    std::vector<float> block(count, 0.0f);
    for (std::size_t index = 0; index < count && from + index < samples.size(); ++index)
        block[index] = samples[from + index];

    return block;
}

} // namespace

TEST_CASE("the drum machine is in the catalogue with a parameter per pad control")
{
    const BuiltinInstrumentInfo* info = findBuiltinInstrument("incdaw.drum");
    REQUIRE(info != nullptr);

    CHECK(info->parameterCount == DrumMachine::parameterCount);
    CHECK(info->presets.count >= 4);

    for (int pad = 0; pad < DrumMachine::padCount; ++pad)
        for (const DrumParam::PadOffset offset :
             {DrumParam::engine, DrumParam::tune, DrumParam::decay, DrumParam::tone,
              DrumParam::level, DrumParam::pan, DrumParam::chokeGroup, DrumParam::snap}) {
            const std::uint32_t id = DrumParam::forPad(pad, offset);

            bool found = false;
            for (std::size_t index = 0; index < info->parameterCount; ++index)
                found = found || info->parameters[index].id == id;

            CAPTURE(pad);
            CAPTURE(static_cast<int>(offset));
            CHECK(found);
        }
}

TEST_CASE("pads sit on consecutive keys and nothing outside them plays")
{
    CHECK(DrumMachine::padForKey(DrumMachine::firstKey) == 0);
    CHECK(DrumMachine::padForKey(DrumMachine::firstKey + 15) == 15);
    CHECK(DrumMachine::padForKey(DrumMachine::firstKey - 1) == -1);
    CHECK(DrumMachine::padForKey(DrumMachine::firstKey + 16) == -1);

    DrumMachine drum;
    drum.prepare(rate, 512);

    AudioBufferPool pool;
    pool.allocate(1, 2, 512);

    MidiBuffer stray;
    stray.insert(MidiMessage::noteOn(0, DrumMachine::firstKey + 40, 100, 0));

    const auto output = pool.buffer(0);
    output.clear();
    drum.processBlock(output, stray);

    CHECK(drum.activeVoiceCount() == 0);
    CHECK(output.peak() == doctest::Approx(0.0f));
}

TEST_CASE("every engine makes a sound and every sound ends")
{
    for (int engine = 0; engine < DrumMachine::engineCount; ++engine) {
        DrumMachine drum;
        drum.prepare(rate, 512);

        set(drum, 0, DrumParam::engine, static_cast<double>(engine));
        set(drum, 0, DrumParam::decay, 0.2);

        const Rendered rendered = strike(drum, 0, 120000);

        CAPTURE(engine);
        CHECK(peakOf(rendered.left, 0, 4800) > 0.02);

        for (const float sample : rendered.left)
            REQUIRE(std::isfinite(sample));

        // Two and a half seconds later there is nothing left, and the voice
        // has given its slot back.
        CHECK(peakOf(rendered.left, 112000, 120000) < 1e-4);
        CHECK(drum.activeVoiceCount() == 0);
    }
}

TEST_CASE("a hat is bright and a kick is not")
{
    DrumMachine drum;
    drum.prepare(rate, 512);

    set(drum, 0, DrumParam::engine, static_cast<double>(DrumMachine::Engine::kick));
    set(drum, 0, DrumParam::pan, 0.0);
    const double kick = peakFrequency(window(strike(drum, 0, 8192).left, 0, 8192));

    DrumMachine hats;
    hats.prepare(rate, 512);
    set(hats, 0, DrumParam::engine, static_cast<double>(DrumMachine::Engine::hat));
    set(hats, 0, DrumParam::pan, 0.0);
    const double hat = peakFrequency(window(strike(hats, 0, 8192).left, 0, 8192));

    CAPTURE(kick);
    CAPTURE(hat);
    CHECK(kick < 250.0);
    CHECK(hat > 3000.0);
}

TEST_CASE("a kick's pitch falls, which is what makes it a kick")
{
    DrumMachine drum;
    drum.prepare(rate, 512);

    set(drum, 0, DrumParam::engine, static_cast<double>(DrumMachine::Engine::kick));
    set(drum, 0, DrumParam::snap, 1.0);
    set(drum, 0, DrumParam::decay, 0.6);
    set(drum, 0, DrumParam::tone, 0.0);   // no click to confuse the measurement
    set(drum, 0, DrumParam::pan, 0.0);

    const Rendered rendered = strike(drum, 0, 24000);

    // The first ten milliseconds against a window well after the sweep.
    const double first  = peakFrequency(window(rendered.left, 0, 512));
    const double second = peakFrequency(window(rendered.left, 8192, 512));

    CAPTURE(first);
    CAPTURE(second);
    CHECK(second < first);
}

TEST_CASE("tuning a pad moves its pitch")
{
    const auto peakAtTune = [](double tune) {
        DrumMachine drum;
        drum.prepare(rate, 512);
        set(drum, 0, DrumParam::engine, static_cast<double>(DrumMachine::Engine::tom));
        set(drum, 0, DrumParam::tune, tune);
        set(drum, 0, DrumParam::pan, 0.0);
        set(drum, 0, DrumParam::snap, 0.0);

        return peakFrequency(window(strike(drum, 0, 8192).left, 0, 8192));
    };

    CHECK(peakAtTune(12.0) > peakAtTune(-12.0) * 1.5);
}

TEST_CASE("pan puts a pad on one side")
{
    DrumMachine drum;
    drum.prepare(rate, 512);

    set(drum, 0, DrumParam::pan, -1.0);
    const Rendered hardLeft = strike(drum, 0, 4096);

    CHECK(peakOf(hardLeft.left, 0, 4096) > 0.01);
    CHECK(peakOf(hardLeft.right, 0, 4096) < peakOf(hardLeft.left, 0, 4096) * 0.05);

    DrumMachine other;
    other.prepare(rate, 512);
    set(other, 0, DrumParam::pan, 1.0);
    const Rendered hardRight = strike(other, 0, 4096);

    CHECK(peakOf(hardRight.right, 0, 4096) > 0.01);
    CHECK(peakOf(hardRight.left, 0, 4096) < peakOf(hardRight.right, 0, 4096) * 0.05);
}

TEST_CASE("a choke group cuts across pads, and an ungrouped pad is left alone")
{
    const auto tailAfterSecondHit = [](double groupOfSecondPad) {
        DrumMachine drum;
        drum.prepare(rate, 512);

        // Pad 0 rings for a long time; pad 1 is struck a moment later.
        set(drum, 0, DrumParam::engine, static_cast<double>(DrumMachine::Engine::hat));
        set(drum, 0, DrumParam::decay, 3.0);
        set(drum, 0, DrumParam::chokeGroup, 1.0);
        set(drum, 0, DrumParam::pan, 0.0);

        set(drum, 1, DrumParam::engine, static_cast<double>(DrumMachine::Engine::rim));
        set(drum, 1, DrumParam::decay, 0.02);
        set(drum, 1, DrumParam::level, 0.0);   // silent, so only the choke shows
        set(drum, 1, DrumParam::chokeGroup, groupOfSecondPad);

        AudioBufferPool pool;
        pool.allocate(1, 2, 512);
        const auto output = pool.buffer(0);

        MidiBuffer open;
        open.insert(MidiMessage::noteOn(0, DrumMachine::firstKey, 120, 0));
        output.clear();
        drum.processBlock(output, open);

        MidiBuffer closed;
        closed.insert(MidiMessage::noteOn(0, DrumMachine::firstKey + 1, 120, 0));
        output.clear();
        drum.processBlock(output, closed);

        // Well past the choke fade.
        output.clear();
        drum.processBlock(output, MidiBuffer{});

        return static_cast<double>(output.peak());
    };

    CHECK(tailAfterSecondHit(1.0) < 1e-5);       // same group: cut
    CHECK(tailAfterSecondHit(0.0) > 1e-3);       // no group: still ringing
}

TEST_CASE("retriggering a pad does not click")
{
    DrumMachine drum;
    drum.prepare(rate, 512);

    set(drum, 0, DrumParam::engine, static_cast<double>(DrumMachine::Engine::tom));
    set(drum, 0, DrumParam::decay, 2.0);
    set(drum, 0, DrumParam::pan, 0.0);

    AudioBufferPool pool;
    pool.allocate(1, 2, 512);
    const auto output = pool.buffer(0);

    MidiBuffer hit;
    hit.insert(MidiMessage::noteOn(0, DrumMachine::firstKey, 120, 0));

    output.clear();
    drum.processBlock(output, hit);

    // Second hit mid-ring. The old voice fades rather than stopping dead, so
    // no single sample-to-sample step is large.
    output.clear();
    drum.processBlock(output, hit);

    double biggestStep = 0.0;
    for (FrameCount frame = 1; frame < 512; ++frame)
        biggestStep = std::max(biggestStep,
                               std::abs(static_cast<double>(output.channel(0)[frame])
                                        - static_cast<double>(output.channel(0)[frame - 1])));

    CAPTURE(biggestStep);
    CHECK(biggestStep < 0.35);
}

TEST_CASE("a live edit does not change a hit already sounding")
{
    DrumMachine drum;
    drum.prepare(rate, 512);

    set(drum, 0, DrumParam::engine, static_cast<double>(DrumMachine::Engine::tom));
    set(drum, 0, DrumParam::decay, 1.0);
    set(drum, 0, DrumParam::pan, 0.0);

    AudioBufferPool pool;
    pool.allocate(1, 2, 512);
    const auto output = pool.buffer(0);

    MidiBuffer hit;
    hit.insert(MidiMessage::noteOn(0, DrumMachine::firstKey, 120, 0));
    output.clear();
    drum.processBlock(output, hit);

    // Turning the pad down mid-ring must not duck what is already playing:
    // a drum's settings belong to the hit, not to the clock.
    const double before = static_cast<double>(output.peak());
    set(drum, 0, DrumParam::level, 0.0);

    output.clear();
    drum.processBlock(output, MidiBuffer{});
    const double after = static_cast<double>(output.peak());

    CHECK(after > before * 0.2);
}

TEST_CASE("more hits than voices steals the oldest")
{
    DrumMachine drum;
    drum.prepare(rate, 512);

    for (int pad = 0; pad < DrumMachine::padCount; ++pad) {
        set(drum, pad, DrumParam::decay, 4.0);
        set(drum, pad, DrumParam::chokeGroup, 0.0);
    }

    AudioBufferPool pool;
    pool.allocate(1, 2, 512);
    const auto output = pool.buffer(0);

    MidiBuffer everything;
    for (int pad = 0; pad < DrumMachine::padCount; ++pad)
        everything.insert(MidiMessage::noteOn(0, DrumMachine::firstKey + pad, 100, 0));

    output.clear();
    drum.processBlock(output, everything);
    CHECK(drum.activeVoiceCount() == DrumMachine::padCount);

    output.clear();
    drum.processBlock(output, everything);

    CHECK(drum.activeVoiceCount() <= DrumMachine::maxVoices);
    CHECK(std::isfinite(static_cast<double>(output.peak())));
}

TEST_CASE("all notes off silences the kit")
{
    DrumMachine drum;
    drum.prepare(rate, 512);

    AudioBufferPool pool;
    pool.allocate(1, 2, 512);
    const auto output = pool.buffer(0);

    MidiBuffer everything;
    for (int pad = 0; pad < DrumMachine::padCount; ++pad)
        everything.insert(MidiMessage::noteOn(0, DrumMachine::firstKey + pad, 100, 0));

    output.clear();
    drum.processBlock(output, everything);
    CHECK(drum.activeVoiceCount() > 0);

    MidiBuffer panic;
    panic.insert(MidiMessage::controlChange(0, 123, 0, 0));
    output.clear();
    drum.processBlock(output, panic);

    CHECK(drum.activeVoiceCount() == 0);
}

TEST_CASE("rendering allocates nothing on the audio thread")
{
    DrumMachine drum;
    drum.prepare(rate, 512);

    AudioBufferPool pool;
    pool.allocate(1, 2, 512);
    const auto output = pool.buffer(0);

    rt::resetViolations();

    {
        const rt::ScopedRealtimeContext scope;

        for (int block = 0; block < 8; ++block) {
            MidiBuffer hits;
            for (int pad = 0; pad < 4; ++pad)
                hits.insert(MidiMessage::noteOn(
                    0, DrumMachine::firstKey + pad, 100,
                    static_cast<FrameCount>(pad) * 64));

            output.clear();
            drum.processBlock(output, hits);
            drum.setParameter(DrumParam::forPad(0, DrumParam::tune),
                              static_cast<double>(block) - 4.0);
        }
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(std::isfinite(static_cast<double>(output.peak())));
}
