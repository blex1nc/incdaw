// Phase 15 — the builtin DSP suite.
//
// The exit criterion (docs/ROADMAP.md): each effect passes a null test
// against its own reference implementation, and all share one interface with
// no special-casing. The reference implementations live HERE, as straight-
// line loops over the same formulas, written independently of the effect
// classes — an effect that drifts from its own definition fails its test.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/dsp/effects/DynamicsEffects.h"
#include "engine/dsp/effects/SpaceEffects.h"
#include "engine/dsp/effects/ToneEffects.h"
#include "engine/dsp/effects/UtilityEffects.h"

#include <cmath>
#include <memory>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
using namespace incdaw::engine::dsp;

namespace {

constexpr FrameCount blockSize  = 256;
constexpr double     sampleRate = 48000.0;

/// A deterministic, broadband, stereo-decorrelated test signal.
std::vector<std::vector<Sample>> testSignal(std::size_t channels, FrameCount frames)
{
    std::vector<std::vector<Sample>> signal(channels);

    std::uint32_t state = 0x12345678u;
    const auto    noise = [&state]() {
        state = state * 1664525u + 1013904223u;
        return (static_cast<double>(state) / 4294967295.0) * 2.0 - 1.0;
    };

    for (std::size_t channel = 0; channel < channels; ++channel) {
        signal[channel].resize(static_cast<std::size_t>(frames));
        for (FrameCount frame = 0; frame < frames; ++frame) {
            const double tone =
                0.4
                * std::sin(2.0 * 3.14159265358979 * (220.0 + 60.0 * static_cast<double>(channel))
                           * static_cast<double>(frame) / sampleRate);
            signal[channel][static_cast<std::size_t>(frame)] =
                static_cast<Sample>(tone + 0.2 * noise());
        }
    }

    return signal;
}

/// Runs `node` over `input` block by block through the real Node contract:
/// the input arrives as a graph input, the output starts silent.
std::vector<std::vector<Sample>> processThrough(Node&                                   node,
                                                const std::vector<std::vector<Sample>>& input)
{
    const std::size_t channels = input.size();
    const auto        frames   = static_cast<FrameCount>(input[0].size());

    node.prepare(sampleRate, blockSize);

    AudioBufferPool pool;
    pool.allocate(2, channels, blockSize);

    std::vector<std::vector<Sample>> output(channels);
    for (auto& channel : output)
        channel.reserve(static_cast<std::size_t>(frames));

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
        context.output       = outputView;
        context.inputs       = &inputView;
        context.inputCount   = 1;
        context.frameCount   = count;
        context.sampleRate   = sampleRate;
        context.playPosition = start;

        node.process(context);

        for (std::size_t channel = 0; channel < channels; ++channel)
            for (FrameCount frame = 0; frame < count; ++frame)
                output[channel].push_back(outputView.channel(channel)[frame]);
    }

    return output;
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

void requireClose(const std::vector<std::vector<Sample>>& a,
                  const std::vector<std::vector<Sample>>& b, double epsilon = 1.0e-6)
{
    REQUIRE(a.size() == b.size());
    for (std::size_t channel = 0; channel < a.size(); ++channel) {
        REQUIRE(a[channel].size() == b[channel].size());
        for (std::size_t frame = 0; frame < a[channel].size(); ++frame)
            REQUIRE(static_cast<double>(a[channel][frame])
                    == doctest::Approx(static_cast<double>(b[channel][frame]))
                           .epsilon(epsilon)
                           .scale(1.0));
    }
}

double db(double gain) { return std::pow(10.0, gain / 20.0); }

} // namespace

// ── The defaulted suite nulls ────────────────────────────────────────────────

TEST_CASE("every effect at its transparent settings passes the signal bit-exact")
{
    const auto input = testSignal(2, 2048);

    const auto nullThrough = [&](std::unique_ptr<Node> node) {
        requireBitExact(processThrough(*node, input), input);
    };

    SUBCASE("utility")  { nullThrough(makeBuiltinEffect("incdaw.utility", 48000.0)); }
    SUBCASE("filter")   { nullThrough(makeBuiltinEffect("incdaw.filter", 48000.0)); }
    SUBCASE("eq")       { nullThrough(makeBuiltinEffect("incdaw.eq", 48000.0)); }
    SUBCASE("saturator"){ nullThrough(makeBuiltinEffect("incdaw.saturator", 48000.0)); }
    SUBCASE("gate")     { nullThrough(makeBuiltinEffect("incdaw.gate", 48000.0)); }
    SUBCASE("analyzer") { nullThrough(makeBuiltinEffect("incdaw.analyzer", 48000.0)); }

    SUBCASE("compressor at ratio 1")
    {
        auto node = makeBuiltinEffect("incdaw.compressor", 48000.0);
        node->parameterSink()->setParameter(CompressorEffect::ratio, 1.0);
        nullThrough(std::move(node));
    }

    SUBCASE("limiter under the ceiling")
    {
        nullThrough(makeBuiltinEffect("incdaw.limiter", 48000.0));
    }

    SUBCASE("delay at mix zero")
    {
        auto node = makeBuiltinEffect("incdaw.delay", 48000.0);
        node->parameterSink()->setParameter(DelayEffect::mix, 0.0);
        nullThrough(std::move(node));
    }

    SUBCASE("reverb at mix zero")
    {
        auto node = makeBuiltinEffect("incdaw.reverb", 48000.0);
        node->parameterSink()->setParameter(ReverbEffect::mix, 0.0);
        nullThrough(std::move(node));
    }
}

// ── Reference implementations ────────────────────────────────────────────────

TEST_CASE("utility matches its reference formula")
{
    const auto input = testSignal(2, 2048);

    UtilityEffect effect;
    effect.setParameter(UtilityEffect::gainDb, -6.0);
    effect.setParameter(UtilityEffect::pan, 0.5);
    effect.setParameter(UtilityEffect::width, 1.5);

    const auto processed = processThrough(effect, input);

    // Reference: width on mid/side, then gain, then normalised constant-power
    // balance.
    constexpr double quarterPi = 0.78539816339744830962;
    const double gain     = db(-6.0);
    const double panLeft  = std::cos(1.5 * quarterPi / 2.0) / std::cos(quarterPi / 2.0);
    const double panRight = std::sin(1.5 * quarterPi / 2.0) / std::sin(quarterPi / 2.0);

    std::vector<std::vector<Sample>> reference(2);
    for (std::size_t frame = 0; frame < input[0].size(); ++frame) {
        const double left  = static_cast<double>(input[0][frame]);
        const double right = static_cast<double>(input[1][frame]);
        const double mid   = (left + right) * 0.5;
        const double side  = (left - right) * 0.5 * 1.5;

        reference[0].push_back(static_cast<Sample>((mid + side) * gain * panLeft));
        reference[1].push_back(static_cast<Sample>((mid - side) * gain * panRight));
    }

    requireClose(processed, reference);
}

TEST_CASE("the filter matches a reference state-variable loop")
{
    const auto input = testSignal(2, 2048);

    FilterEffect effect;
    effect.setParameter(FilterEffect::mode, static_cast<double>(FilterEffect::lowpass));
    effect.setParameter(FilterEffect::cutoffHz, 800.0);
    effect.setParameter(FilterEffect::resonance, 1.2);

    const auto processed = processThrough(effect, input);

    const double f = 2.0 * std::sin(3.14159265358979323846 * 800.0 / sampleRate);
    const double q = 1.0 / 1.2;

    std::vector<std::vector<Sample>> reference(2);
    for (std::size_t channel = 0; channel < 2; ++channel) {
        double low = 0.0, band = 0.0;
        for (std::size_t frame = 0; frame < input[channel].size(); ++frame) {
            const double in = static_cast<double>(input[channel][frame]);
            low += f * band;
            const double high = in - low - q * band;
            band += f * high;
            reference[channel].push_back(static_cast<Sample>(low));
        }
    }

    requireClose(processed, reference);
}

TEST_CASE("the EQ matches a reference biquad cascade")
{
    const auto input = testSignal(1, 2048);

    EqEffect effect;
    effect.setParameter(EqEffect::midGainDb, 6.0);
    effect.setParameter(EqEffect::midFreq, 1000.0);
    effect.setParameter(EqEffect::midQ, 1.0);

    const auto processed = processThrough(effect, input);

    // Reference: the RBJ peaking band alone (both shelves sit at 0 dB and are
    // skipped as identity).
    const double a     = std::pow(10.0, 6.0 / 40.0);
    const double omega = 2.0 * 3.14159265358979323846 * 1000.0 / sampleRate;
    const double alpha = std::sin(omega) / 2.0;
    const double cosw  = std::cos(omega);
    const double a0    = 1.0 + alpha / a;

    const double b0 = (1.0 + alpha * a) / a0;
    const double b1 = (-2.0 * cosw) / a0;
    const double b2 = (1.0 - alpha * a) / a0;
    const double a1 = (-2.0 * cosw) / a0;
    const double a2 = (1.0 - alpha / a) / a0;

    std::vector<std::vector<Sample>> reference(1);
    double z1 = 0.0, z2 = 0.0;
    for (std::size_t frame = 0; frame < input[0].size(); ++frame) {
        const double in  = static_cast<double>(input[0][frame]);
        const double out = b0 * in + z1;
        z1 = b1 * in - a1 * out + z2;
        z2 = b2 * in - a2 * out;
        reference[0].push_back(static_cast<Sample>(out));
    }

    requireClose(processed, reference);
}

TEST_CASE("the saturator matches tanh(x·g)/g")
{
    const auto input = testSignal(2, 1024);

    SaturatorEffect effect;
    effect.setParameter(SaturatorEffect::driveDb, 12.0);
    effect.setParameter(SaturatorEffect::mix, 0.8);

    const auto processed = processThrough(effect, input);

    const double gain = db(12.0);

    std::vector<std::vector<Sample>> reference(2);
    for (std::size_t channel = 0; channel < 2; ++channel)
        for (std::size_t frame = 0; frame < input[channel].size(); ++frame) {
            const double dry    = static_cast<double>(input[channel][frame]);
            const double shaped = std::tanh(dry * gain) / gain;
            reference[channel].push_back(static_cast<Sample>(shaped * 0.8 + dry * 0.2));
        }

    requireClose(processed, reference);
}

TEST_CASE("the compressor matches a reference gain computer")
{
    const auto input = testSignal(2, 4096);

    CompressorEffect effect;
    effect.setParameter(CompressorEffect::thresholdDb, -18.0);
    effect.setParameter(CompressorEffect::ratio, 4.0);
    effect.setParameter(CompressorEffect::attackMs, 5.0);
    effect.setParameter(CompressorEffect::releaseMs, 80.0);
    effect.setParameter(CompressorEffect::makeupDb, 3.0);

    const auto processed = processThrough(effect, input);

    const double attack  = std::exp(-1.0 / (5.0 * 0.001 * sampleRate));
    const double release = std::exp(-1.0 / (80.0 * 0.001 * sampleRate));
    const double slope   = 1.0 / 4.0 - 1.0;
    const double makeup  = db(3.0);

    std::vector<std::vector<Sample>> reference(2);
    for (auto& channel : reference)
        channel.resize(input[0].size());

    double envelope = 1.0;
    for (std::size_t frame = 0; frame < input[0].size(); ++frame) {
        const double peak = std::max(std::fabs(static_cast<double>(input[0][frame])),
                                     std::fabs(static_cast<double>(input[1][frame])));
        const double peakDb = peak > 1.0e-10 ? 20.0 * std::log10(peak) : -200.0;
        const double overDb = peakDb - (-18.0);
        const double target = db(overDb > 0.0 ? overDb * slope : 0.0);

        const double coefficient = target < envelope ? attack : release;
        envelope = target + coefficient * (envelope - target);

        for (std::size_t channel = 0; channel < 2; ++channel)
            reference[channel][frame] = static_cast<Sample>(
                static_cast<double>(input[channel][frame]) * envelope * makeup);
    }

    requireClose(processed, reference);
}

TEST_CASE("the limiter matches its reference and holds the ceiling")
{
    // A signal that pushes well past the ceiling.
    auto input = testSignal(2, 4096);
    for (auto& channel : input)
        for (auto& sample : channel)
            sample = static_cast<Sample>(static_cast<double>(sample) * 3.0);

    LimiterEffect effect;
    effect.setParameter(LimiterEffect::ceilingDb, -3.0);
    effect.setParameter(LimiterEffect::releaseMs, 50.0);

    const auto processed = processThrough(effect, input);

    const double ceiling = db(-3.0);
    const double release = std::exp(-1.0 / (50.0 * 0.001 * sampleRate));

    std::vector<std::vector<Sample>> reference(2);
    for (auto& channel : reference)
        channel.resize(input[0].size());

    double gain = 1.0;
    for (std::size_t frame = 0; frame < input[0].size(); ++frame) {
        const double peak = std::max(std::fabs(static_cast<double>(input[0][frame])),
                                     std::fabs(static_cast<double>(input[1][frame])));

        gain = 1.0 + release * (gain - 1.0);

        if (peak * gain > ceiling && peak > 0.0)
            gain = ceiling / peak;

        for (std::size_t channel = 0; channel < 2; ++channel)
            reference[channel][frame] =
                gain < 1.0 ? static_cast<Sample>(static_cast<double>(input[channel][frame]) * gain)
                           : input[channel][frame];
    }

    requireClose(processed, reference);

    // And the point of a limiter: nothing escapes the ceiling.
    for (const auto& channel : processed)
        for (const Sample sample : channel)
            REQUIRE(std::fabs(static_cast<double>(sample)) <= ceiling + 1.0e-6);
}

TEST_CASE("the gate matches a reference open/hold/release loop")
{
    // Bursts with silence between: the shape a gate exists for.
    std::vector<std::vector<Sample>> input(1);
    input[0].resize(8192, Sample{0});
    for (std::size_t frame = 0; frame < 8192; ++frame)
        if ((frame / 1024) % 2 == 0)
            input[0][frame] = static_cast<Sample>(
                0.5 * std::sin(2.0 * 3.14159265358979 * 330.0 * static_cast<double>(frame)
                               / sampleRate));

    GateEffect effect;
    effect.setParameter(GateEffect::thresholdDb, -30.0);
    effect.setParameter(GateEffect::attackMs, 1.0);
    effect.setParameter(GateEffect::holdMs, 5.0);
    effect.setParameter(GateEffect::releaseMs, 20.0);

    const auto processed = processThrough(effect, input);

    const double threshold  = db(-30.0);
    const double attack     = std::exp(-1.0 / (1.0 * 0.001 * sampleRate));
    const double release    = std::exp(-1.0 / (20.0 * 0.001 * sampleRate));
    const double holdFrames = 5.0 * 0.001 * sampleRate;

    std::vector<std::vector<Sample>> reference(1);
    reference[0].resize(input[0].size());

    double gain = 1.0, holdLeft = 0.0;
    for (std::size_t frame = 0; frame < input[0].size(); ++frame) {
        const double peak = std::fabs(static_cast<double>(input[0][frame]));

        double target;
        if (peak >= threshold) {
            target   = 1.0;
            holdLeft = holdFrames;
        } else if (holdLeft > 0.0) {
            target = 1.0;
            holdLeft -= 1.0;
        } else {
            target = 0.0;
        }

        const double coefficient = target > gain ? attack : release;
        gain = target + coefficient * (gain - target);

        reference[0][frame] =
            static_cast<Sample>(static_cast<double>(input[0][frame]) * gain);
    }

    requireClose(processed, reference);
}

TEST_CASE("the delay matches a reference feedback line")
{
    const auto input = testSignal(2, 8192);

    DelayEffect effect;
    effect.setParameter(DelayEffect::timeMs, 100.0);
    effect.setParameter(DelayEffect::feedback, 0.5);
    effect.setParameter(DelayEffect::mix, 0.6);

    const auto processed = processThrough(effect, input);

    const auto delayFrames = static_cast<std::size_t>(100.0 * 0.001 * sampleRate);

    std::vector<std::vector<Sample>> reference(2);
    for (std::size_t channel = 0; channel < 2; ++channel) {
        std::vector<Sample> line(input[channel].size() + delayFrames, Sample{0});
        reference[channel].resize(input[channel].size());

        for (std::size_t frame = 0; frame < input[channel].size(); ++frame) {
            const double dry     = static_cast<double>(input[channel][frame]);
            const double delayed = frame >= delayFrames
                                       ? static_cast<double>(line[frame - delayFrames])
                                       : 0.0;

            line[frame] = static_cast<Sample>(dry + delayed * 0.5);

            reference[channel][frame] = static_cast<Sample>(dry + delayed * 0.6);
        }
    }

    requireClose(processed, reference);
}

TEST_CASE("the reverb matches a reference Schroeder network")
{
    const auto input = testSignal(1, 4096);

    ReverbEffect effect;
    effect.setParameter(ReverbEffect::size, 0.8);
    effect.setParameter(ReverbEffect::damping, 0.4);
    effect.setParameter(ReverbEffect::mix, 0.5);

    const auto processed = processThrough(effect, input);

    // The reference network, independently: same tunings scaled to 48 kHz
    // (scale 1), same topology.
    constexpr std::size_t combLengths[4]    = {1557, 1617, 1491, 1422};
    constexpr std::size_t allpassLengths[2] = {225, 556};

    const double feedback = 0.7 + 0.8 * 0.18;
    const double damp     = 0.4 * 0.8;

    struct RefComb {
        std::vector<double> line;
        std::size_t         index = 0;
        double              store = 0.0;
    } combs[4];

    struct RefAllpass {
        std::vector<double> line;
        std::size_t         index = 0;
    } allpasses[2];

    for (std::size_t comb = 0; comb < 4; ++comb)
        combs[comb].line.assign(combLengths[comb], 0.0);
    for (std::size_t allpass = 0; allpass < 2; ++allpass)
        allpasses[allpass].line.assign(allpassLengths[allpass], 0.0);

    std::vector<std::vector<Sample>> reference(1);
    reference[0].resize(input[0].size());

    for (std::size_t frame = 0; frame < input[0].size(); ++frame) {
        const double dry   = static_cast<double>(input[0][frame]);
        const double drive = dry * 0.2;

        double wet = 0.0;
        for (RefComb& comb : combs) {
            const double out = comb.line[comb.index];
            wet += out;
            comb.store           = out * (1.0 - damp) + comb.store * damp;
            // The effect stores through float; mirror that rounding.
            comb.line[comb.index] =
                static_cast<double>(static_cast<Sample>(drive + comb.store * feedback));
            if (++comb.index >= comb.line.size())
                comb.index = 0;
        }

        for (RefAllpass& allpass : allpasses) {
            const double buffered = allpass.line[allpass.index];
            const double output   = -wet + buffered;
            allpass.line[allpass.index] =
                static_cast<double>(static_cast<Sample>(wet + buffered * 0.5));
            if (++allpass.index >= allpass.line.size())
                allpass.index = 0;
            wet = output;
        }

        reference[0][frame] = static_cast<Sample>(dry + wet * 0.5);
    }

    requireClose(processed, reference, 1.0e-5);
}

TEST_CASE("the analyzer measures without touching the signal")
{
    const auto input = testSignal(2, 1024);

    AnalyzerEffect effect;
    const auto processed = processThrough(effect, input);

    requireBitExact(processed, input);

    // The published numbers are the last block's.
    float  expectedPeak = 0.0f;
    double sum          = 0.0;
    for (std::size_t frame = input[0].size() - blockSize; frame < input[0].size(); ++frame) {
        expectedPeak = std::max(expectedPeak, std::fabs(input[0][frame]));
        sum += static_cast<double>(input[0][frame]) * static_cast<double>(input[0][frame]);
    }

    CHECK(effect.peak(0) == doctest::Approx(static_cast<double>(expectedPeak)));
    CHECK(effect.rms(0)
          == doctest::Approx(std::sqrt(sum / static_cast<double>(blockSize))).epsilon(0.001));
}

// ── The lookahead limiter ────────────────────────────────────────────────────

TEST_CASE("the lookahead limiter is a pure delay when transparent")
{
    const auto input = testSignal(2, 2048);

    engine::dsp::LookaheadLimiterEffect transparent{48000.0};
    const auto lookahead = static_cast<std::size_t>(transparent.latencyFrames());
    CHECK(lookahead == 96);   // 2 ms at 48 kHz — and what it TELLS the graph

    const auto delayed = processThrough(transparent, input);

    for (std::size_t channel = 0; channel < 2; ++channel) {
        for (std::size_t frame = 0; frame < lookahead; ++frame)
            REQUIRE(delayed[channel][frame] == 0.0f);

        for (std::size_t frame = lookahead; frame < delayed[channel].size(); ++frame)
            REQUIRE(delayed[channel][frame] == input[channel][frame - lookahead]);
    }
}

TEST_CASE("the lookahead limiter holds its ceiling through a step transient")
{
    // The zero-lookahead limiter clamps the first hot sample by force; this
    // one must have ramped DOWN before the step even arrives at the output.
    std::vector<std::vector<Sample>> step(2, std::vector<Sample>(2048, 0.0f));
    for (auto& channel : step)
        for (std::size_t frame = 256; frame < channel.size(); ++frame)
            channel[frame] = 1.0f;

    engine::dsp::LookaheadLimiterEffect limiter{48000.0};
    limiter.setParameter(engine::dsp::LookaheadLimiterEffect::ceilingDb, -6.0);
    limiter.setParameter(engine::dsp::LookaheadLimiterEffect::releaseMs, 200.0);

    const auto out     = processThrough(limiter, step);
    const auto ceiling = static_cast<Sample>(db(-6.0));

    Sample peak = 0.0f;
    for (const auto& channel : out)
        for (const Sample sample : channel)
            peak = std::max(peak, std::abs(sample));

    CHECK(peak <= ceiling * 1.0001f);   // never over, first transient included
    CHECK(peak >= ceiling * 0.9f);      // and it genuinely limits, not mutes
}

// ── The analyzer's spectrum ──────────────────────────────────────────────────

TEST_CASE("the analyzer publishes a spectrum whose peak sits at the tone's bin")
{
    engine::dsp::AnalyzerEffect analyzer;

    // Nothing published before a full window.
    std::vector<float> bins;
    CHECK_FALSE(analyzer.readSpectrum(bins));

    // A full-scale sine exactly on bin 32: 32 * 48000 / 2048 = 750 Hz.
    constexpr std::size_t toneBin = 32;
    const std::size_t     size    = engine::dsp::AnalyzerEffect::fftSize;

    std::vector<std::vector<Sample>> input(
        2, std::vector<Sample>(size, 0.0f));
    for (std::size_t frame = 0; frame < size; ++frame) {
        const auto value = static_cast<Sample>(
            std::sin(2.0 * M_PI * toneBin * static_cast<double>(frame)
                     / static_cast<double>(size)));
        input[0][frame] = value;
        input[1][frame] = value;
    }

    (void)processThrough(analyzer, input);

    REQUIRE(analyzer.readSpectrum(bins));
    REQUIRE(bins.size() == engine::dsp::AnalyzerEffect::binCount);

    // The peak bin is the tone's, at ~0 dBFS; far bins sit deep below.
    std::size_t peakBin = 0;
    for (std::size_t bin = 1; bin < bins.size(); ++bin)
        if (bins[bin] > bins[peakBin])
            peakBin = bin;

    CHECK(peakBin == toneBin);
    CHECK(bins[toneBin] > -1.0f);
    CHECK(bins[toneBin] < 1.0f);
    CHECK(bins[toneBin + 40] < -40.0f);
}
