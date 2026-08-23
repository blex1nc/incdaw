// Bar-synced time and volume gating (A13).
//
// Two claims, and both are about SYNC rather than about DSP: the pattern must
// land where the bar lands, and it must stay there when the tempo changes —
// a gate written on the sixteenths that drifts is not a gate, it is a
// tremolo. So the tests drive the effect with a real tempo map and a known
// play position and check the gain at named points in the bar.
//
// The time half is checked the way a delay is: feed a click, and see where it
// comes out.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/effects/BeatGate.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/transport/TempoMap.h"

#include <cmath>
#include <memory>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
using namespace incdaw::engine::dsp;

namespace {

constexpr FrameCount blockSize  = 256;
constexpr double     sampleRate = 48000.0;

using Gate = BeatGateEffect;

TempoMap mapAt(double bpm)
{
    return TempoMap{bpm, sampleRate};
}

/// Runs `frames` of a constant signal and hands back what came out.
std::vector<Sample> runConstant(BeatGateEffect& gate, double level, std::size_t frames,
                                FramePosition startPosition = 0)
{
    gate.prepare(sampleRate, blockSize);

    AudioBufferPool pool;
    pool.allocate(2, 1, blockSize);

    std::vector<Sample> output;
    output.reserve(frames);

    for (std::size_t start = 0; start < frames; start += blockSize) {
        const auto count =
            static_cast<FrameCount>(std::min<std::size_t>(blockSize, frames - start));

        const AudioBufferView inputView  = pool.buffer(0);
        const AudioBufferView outputView = pool.buffer(1);

        inputView.clear();
        outputView.clear();

        for (FrameCount frame = 0; frame < count; ++frame)
            inputView.channel(0)[frame] = static_cast<Sample>(level);

        const AudioBufferView inputs[] = {inputView.subBlock(0, count)};

        ProcessContext context{};
        context.output       = outputView.subBlock(0, count);
        context.inputs       = inputs;
        context.inputCount   = 1;
        context.frameCount   = count;
        context.sampleRate   = sampleRate;
        context.playPosition = startPosition + static_cast<FramePosition>(start);
        context.playing      = true;

        gate.process(context);

        for (FrameCount frame = 0; frame < count; ++frame)
            output.push_back(outputView.channel(0)[frame]);
    }

    return output;
}

} // namespace

TEST_CASE("the beat gate is in the catalogue with both curves")
{
    const BuiltinEffectInfo* info = findBuiltinEffect("incdaw.beatgate");
    REQUIRE(info != nullptr);

    CHECK(info->parameterCount == 5 + beatGatePoints * 2);
    CHECK(info->presets.count >= 3);

    for (std::size_t index = 0; index < beatGatePoints; ++index) {
        bool time = false;
        bool volume = false;

        for (std::size_t which = 0; which < info->parameterCount; ++which) {
            time = time
                || info->parameters[which].id
                       == Gate::timeBase + static_cast<std::uint32_t>(index);
            volume = volume
                || info->parameters[which].id
                       == Gate::volumeBase + static_cast<std::uint32_t>(index);
        }

        CAPTURE(index);
        CHECK(time);
        CHECK(volume);
    }
}

TEST_CASE("the curve passes through its points and loops without a step")
{
    double points[beatGatePoints]{};
    for (std::size_t index = 0; index < beatGatePoints; ++index)
        points[index] = static_cast<double>(index) / static_cast<double>(beatGatePoints);

    for (std::size_t index = 0; index < beatGatePoints; ++index) {
        const double phase = static_cast<double>(index) / static_cast<double>(beatGatePoints);
        CAPTURE(index);
        CHECK(beatGateCurveAt(points, phase) == doctest::Approx(points[index]));
    }

    // Just before the bar line and just after it are neighbours, because the
    // last point interpolates back to the first.
    CHECK(std::fabs(beatGateCurveAt(points, 0.9999) - beatGateCurveAt(points, 0.0)) < 0.07);

    // Out-of-range phases wrap rather than reading off the end.
    CHECK(beatGateCurveAt(points, 2.25) == doctest::Approx(beatGateCurveAt(points, 0.25)));
    CHECK(std::isfinite(beatGateCurveAt(points, std::nan(""))));
}

TEST_CASE("with no tempo map there is nothing to sync to, and nothing happens")
{
    BeatGateEffect gate;   // no map
    gate.setParameter(Gate::mix, 1.0);
    gate.setParameter(Gate::volumeBase + 0, 0.0);

    const std::vector<Sample> output = runConstant(gate, 0.5, 2048);

    for (const Sample sample : output)
        REQUIRE(sample == doctest::Approx(0.5f));
}

TEST_CASE("the volume curve lands where the bar says it should")
{
    const TempoMap map = mapAt(120.0);

    // 120 bpm, four-four: a bar is two seconds, a sixteenth is 6000 frames.
    BeatGateEffect gate{&map};
    gate.setParameter(Gate::mix, 1.0);
    gate.setParameter(Gate::smoothingMs, 0.0);

    // Silence the first sixteenth and nothing else.
    gate.setParameter(Gate::volumeBase + 0, 0.0);

    // A bar at 120 bpm four-four is two seconds — 96000 frames — so a
    // sixteenth is 6000 and the pattern repeats at 96000.
    const std::vector<Sample> output = runConstant(gate, 0.5, 140000);

    // Inside the first sixteenth: gated.
    CHECK(std::fabs(static_cast<double>(output[100])) < 0.02);

    // In the middle of the second sixteenth, well past the ramp back up.
    CHECK(static_cast<double>(output[9000]) == doctest::Approx(0.5).epsilon(0.05));

    // And it comes back on the next bar line.
    CHECK(std::fabs(static_cast<double>(output[96100])) < 0.02);
    CHECK(static_cast<double>(output[105000]) == doctest::Approx(0.5).epsilon(0.05));
}

TEST_CASE("the pattern follows the tempo rather than the clock")
{
    // The same curve at two tempos must gate the same MUSICAL position, which
    // is a different number of frames.
    const auto firstSilenceEnd = [](double bpm) {
        const TempoMap map = mapAt(bpm);

        BeatGateEffect gate{&map};
        gate.setParameter(Gate::mix, 1.0);
        gate.setParameter(Gate::smoothingMs, 0.0);
        gate.setParameter(Gate::volumeBase + 0, 0.0);

        const std::vector<Sample> output = runConstant(gate, 0.5, 96000);

        for (std::size_t index = 0; index < output.size(); ++index)
            if (std::fabs(static_cast<double>(output[index])) > 0.25)
                return index;

        return output.size();
    };

    const std::size_t slow = firstSilenceEnd(60.0);
    const std::size_t fast = firstSilenceEnd(120.0);

    CAPTURE(slow);
    CAPTURE(fast);

    // Twice the tempo, half the frames — a sixteenth is a sixteenth.
    CHECK(static_cast<double>(slow) == doctest::Approx(static_cast<double>(fast) * 2.0)
                                           .epsilon(0.1));
}

TEST_CASE("a flat time curve at zero reads the present")
{
    const TempoMap map = mapAt(120.0);

    BeatGateEffect gate{&map};
    gate.setParameter(Gate::mix, 1.0);
    gate.setParameter(Gate::smoothingMs, 0.0);

    const std::vector<Sample> output = runConstant(gate, 0.5, 8192);

    for (std::size_t index = 1; index < output.size(); ++index)
        REQUIRE(static_cast<double>(output[index]) == doctest::Approx(0.5).epsilon(0.001));
}

TEST_CASE("the time curve reads back into the buffer by the amount it names")
{
    const TempoMap map = mapAt(120.0);

    BeatGateEffect gate{&map};
    gate.setParameter(Gate::mix, 1.0);
    gate.setParameter(Gate::smoothingMs, 0.0);

    // An eighth of a bar, everywhere: a constant delay of 12000 frames at
    // 120 bpm four-four.
    for (std::size_t index = 0; index < beatGatePoints; ++index)
        gate.setParameter(Gate::timeBase + static_cast<std::uint32_t>(index), 0.125);

    gate.prepare(sampleRate, blockSize);

    AudioBufferPool pool;
    pool.allocate(2, 1, blockSize);

    // A click at the very start, then silence: where it comes out is the
    // delay the curve asked for.
    std::vector<Sample> output;
    constexpr std::size_t frames = 32768;

    for (std::size_t start = 0; start < frames; start += blockSize) {
        const AudioBufferView inputView  = pool.buffer(0);
        const AudioBufferView outputView = pool.buffer(1);

        inputView.clear();
        outputView.clear();

        if (start == 0)
            inputView.channel(0)[0] = 1.0f;

        const AudioBufferView inputs[] = {inputView};

        ProcessContext context{};
        context.output       = outputView;
        context.inputs       = inputs;
        context.inputCount   = 1;
        context.frameCount   = blockSize;
        context.sampleRate   = sampleRate;
        context.playPosition = static_cast<FramePosition>(start);
        context.playing      = true;

        gate.process(context);

        for (FrameCount frame = 0; frame < blockSize; ++frame)
            output.push_back(outputView.channel(0)[frame]);
    }

    std::size_t peakAt = 0;
    double      peak   = 0.0;

    for (std::size_t index = 0; index < output.size(); ++index)
        if (std::fabs(static_cast<double>(output[index])) > peak) {
            peak   = std::fabs(static_cast<double>(output[index]));
            peakAt = index;
        }

    CAPTURE(peakAt);
    CHECK(peak > 0.3);

    // 0.125 bars at 120 bpm four-four is a quarter of a second.
    CHECK(static_cast<double>(peakAt) == doctest::Approx(12000.0).epsilon(0.02));
}

TEST_CASE("mix blends the shifted signal back against the original")
{
    const TempoMap map = mapAt(120.0);

    BeatGateEffect gate{&map};
    gate.setParameter(Gate::mix, 0.5);
    gate.setParameter(Gate::smoothingMs, 0.0);

    for (std::size_t index = 0; index < beatGatePoints; ++index)
        gate.setParameter(Gate::volumeBase + static_cast<std::uint32_t>(index), 0.0);

    // Every point silent, half wet: the output is half the input.
    const std::vector<Sample> output = runConstant(gate, 0.8, 4096);

    for (std::size_t index = 64; index < output.size(); ++index)
        REQUIRE(static_cast<double>(output[index]) == doctest::Approx(0.4).epsilon(0.01));
}
