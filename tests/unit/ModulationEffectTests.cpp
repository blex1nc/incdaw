#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/dsp/effects/DynamicsEffects.h"
#include "engine/dsp/effects/ModulationEffects.h"

#include <cmath>
#include <memory>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
using namespace incdaw::engine::dsp;

namespace {

constexpr FrameCount blockSize  = 256;
constexpr double     sampleRate = 48000.0;

std::vector<std::vector<Sample>> testSignal(FrameCount frames)
{
    std::vector<std::vector<Sample>> signal(2);

    std::uint32_t state = 0x2468ACE1u;
    const auto    noise = [&state]() {
        state = state * 1664525u + 1013904223u;
        return (static_cast<double>(state) / 4294967295.0) * 2.0 - 1.0;
    };

    for (std::size_t channel = 0; channel < 2; ++channel) {
        signal[channel].resize(static_cast<std::size_t>(frames));
        for (FrameCount frame = 0; frame < frames; ++frame) {
            const double tone =
                0.4 * std::sin(2.0 * 3.14159265358979 * (330.0 + 40.0 * static_cast<double>(channel))
                               * static_cast<double>(frame) / sampleRate);
            signal[channel][static_cast<std::size_t>(frame)] =
                static_cast<Sample>(tone + 0.1 * noise());
        }
    }

    return signal;
}

std::vector<std::vector<Sample>> processThrough(Node& node,
                                                const std::vector<std::vector<Sample>>& input)
{
    const std::size_t channels = input.size();
    const auto        frames   = static_cast<FrameCount>(input[0].size());

    node.prepare(sampleRate, blockSize);

    AudioBufferPool pool;
    pool.allocate(2, channels, blockSize);

    std::vector<std::vector<Sample>> output(channels);

    for (FrameCount start = 0; start < frames; start += blockSize) {
        const FrameCount count = std::min<FrameCount>(blockSize, frames - start);

        const AudioBufferView inputView  = pool.buffer(0);
        const AudioBufferView outputView = pool.buffer(1);
        inputView.clear();
        outputView.clear();

        for (std::size_t channel = 0; channel < channels; ++channel)
            for (FrameCount frame = 0; frame < count; ++frame)
                inputView.channel(channel)[frame] =
                    input[channel][static_cast<std::size_t>(start + frame)];

        ProcessContext context;
        context.output     = outputView;
        context.inputs     = &inputView;
        context.inputCount = 1;
        context.frameCount = count;
        context.sampleRate = sampleRate;

        node.process(context);

        for (std::size_t channel = 0; channel < channels; ++channel)
            for (FrameCount frame = 0; frame < count; ++frame)
                output[channel].push_back(outputView.channel(channel)[frame]);
    }

    return output;
}

double rmsOf(const std::vector<Sample>& samples, std::size_t from, std::size_t to)
{
    double sum = 0.0;
    for (std::size_t index = from; index < to; ++index)
        sum += static_cast<double>(samples[index]) * static_cast<double>(samples[index]);
    return std::sqrt(sum / static_cast<double>(to - from));
}

void requireBitExact(const std::vector<std::vector<Sample>>& a,
                     const std::vector<std::vector<Sample>>& b)
{
    REQUIRE(a.size() == b.size());
    for (std::size_t channel = 0; channel < a.size(); ++channel) {
        REQUIRE(a[channel].size() == b[channel].size());
        for (std::size_t frame = 0; frame < a[channel].size(); ++frame)
            REQUIRE(a[channel][frame] == b[channel][frame]);
    }
}

void requireFiniteAndBounded(const std::vector<std::vector<Sample>>& output)
{
    // A resonant feedback path legitimately rings well above the dry level
    // (0.7 feedback approaches 1/(1-0.7) ≈ 3.3× at the resonant comb); the
    // bound catches instability, not character.
    for (const auto& channel : output)
        for (const Sample sample : channel) {
            REQUIRE(std::isfinite(sample));
            REQUIRE(std::fabs(sample) < 8.0f);
        }
}

} // namespace

// ── Null tests (the suite's convention: defaults are transparent) ─────────────

TEST_CASE("modulation effects and the splitter null at their defaults")
{
    const auto input = testSignal(static_cast<FrameCount>(sampleRate));

    ChorusEffect chorus;
    requireBitExact(processThrough(chorus, input), input);

    FlangerEffect flanger;
    requireBitExact(processThrough(flanger, input), input);

    PhaserEffect phaser;
    requireBitExact(processThrough(phaser, input), input);

    TransientSplitEffect splitter;
    requireBitExact(processThrough(splitter, input), input);
}

// ── Engaged behaviour ─────────────────────────────────────────────────────────

TEST_CASE("engaged modulation effects change the signal and stay bounded")
{
    const auto input = testSignal(static_cast<FrameCount>(sampleRate * 2.0));

    ChorusEffect chorus;
    chorus.setParameter(ChorusEffect::mix, 1.0);
    const auto chorused = processThrough(chorus, input);
    requireFiniteAndBounded(chorused);
    CHECK(chorused[0] != input[0]);

    FlangerEffect flanger;
    flanger.setParameter(FlangerEffect::mix, 1.0);
    flanger.setParameter(FlangerEffect::feedback, 0.7);
    const auto flanged = processThrough(flanger, input);
    requireFiniteAndBounded(flanged);
    CHECK(flanged[0] != input[0]);

    PhaserEffect phaser;
    phaser.setParameter(PhaserEffect::mix, 1.0);
    const auto phased = processThrough(phaser, input);
    requireFiniteAndBounded(phased);
    CHECK(phased[0] != input[0]);
}

TEST_CASE("the splitter separates a struck note into transient and sustain")
{
    // Silence, then a sudden sustained tone: an onset followed by steady state.
    const auto totalFrames = static_cast<FrameCount>(sampleRate);
    std::vector<std::vector<Sample>> input(2);
    const auto onset = static_cast<std::size_t>(sampleRate * 0.5);

    for (std::size_t channel = 0; channel < 2; ++channel) {
        input[channel].resize(static_cast<std::size_t>(totalFrames), Sample{0});
        for (std::size_t frame = onset; frame < input[channel].size(); ++frame)
            input[channel][frame] = static_cast<Sample>(
                0.5 * std::sin(2.0 * 3.14159265358979 * 440.0
                               * static_cast<double>(frame - onset) / sampleRate));
    }

    const std::size_t attackEnd  = onset + static_cast<std::size_t>(sampleRate * 0.005);
    const std::size_t steadyFrom = onset + static_cast<std::size_t>(sampleRate * 0.3);
    const std::size_t steadyTo   = static_cast<std::size_t>(totalFrames);

    // Transients only: the onset survives, the steady tail dies.
    TransientSplitEffect transients;
    transients.setParameter(TransientSplitEffect::output, 1.0);
    const auto attack = processThrough(transients, input);

    const double attackBurst  = rmsOf(attack[0], onset, attackEnd);
    const double attackSteady = rmsOf(attack[0], steadyFrom, steadyTo);
    CHECK(attackBurst > 0.05);
    CHECK(attackSteady < 0.02);
    CHECK(attackBurst > attackSteady * 10.0);

    // Sustain only: the steady tail survives nearly untouched.
    TransientSplitEffect sustain;
    sustain.setParameter(TransientSplitEffect::output, 2.0);
    const auto body = processThrough(sustain, input);

    const double inputSteady = rmsOf(input[0], steadyFrom, steadyTo);
    const double bodySteady  = rmsOf(body[0], steadyFrom, steadyTo);
    CHECK(bodySteady > inputSteady * 0.85);

    // Both halves at 0 dB reassemble the note exactly.
    TransientSplitEffect unity;
    requireBitExact(processThrough(unity, input), input);
}

TEST_CASE("the new effects are in the catalogue")
{
    for (const char* uid :
         { "incdaw.chorus", "incdaw.flanger", "incdaw.phaser", "incdaw.transientsplit",
           "incdaw.loudness" }) {
        CHECK(makeBuiltinEffect(uid) != nullptr);
        CHECK(findBuiltinEffect(uid) != nullptr);
    }
}
