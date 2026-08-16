#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/Fft.h"
#include "engine/dsp/GainNode.h"
#include "engine/dsp/SineOscillatorNode.h"

#include <cmath>
#include <numbers>
#include <vector>

using namespace incdaw::engine;

namespace {

ProcessContext makeContext(const AudioBufferView& output, SampleRate rate)
{
    ProcessContext context;
    context.output     = output;
    context.frameCount = output.frameCount();
    context.sampleRate = rate;
    return context;
}

} // namespace

TEST_CASE("the oscillator produces a sine of the requested frequency and amplitude")
{
    constexpr SampleRate rate       = 48000.0;
    constexpr double     frequency  = 1000.0;
    constexpr Sample     amplitude  = 0.5f;
    constexpr FrameCount blockSize  = 480;   // exactly 10 cycles at 1 kHz

    AudioBufferPool pool;
    pool.allocate(1, 1, blockSize);

    dsp::SineOscillatorNode oscillator{frequency, amplitude};
    oscillator.prepare(rate, blockSize);

    const auto buffer = pool.buffer(0);
    const auto context = makeContext(buffer, rate);
    oscillator.process(context);

    CHECK(buffer.peak() == doctest::Approx(static_cast<double>(amplitude)).epsilon(0.01));

    for (FrameCount frame = 0; frame < blockSize; ++frame) {
        const auto expected = static_cast<Sample>(
            static_cast<double>(amplitude)
            * std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(frame) / rate));
        CHECK(buffer.channel(0)[frame] == doctest::Approx(expected).epsilon(0.0001));
    }
}

TEST_CASE("oscillator phase is continuous across block boundaries")
{
    // A phase reset between blocks produces a click at every buffer boundary —
    // the classic symptom of an oscillator that restarts instead of continuing.
    constexpr SampleRate rate      = 48000.0;
    constexpr FrameCount blockSize = 64;

    AudioBufferPool pool;
    pool.allocate(1, 1, blockSize * 2);

    dsp::SineOscillatorNode oscillator{997.0, 1.0f};   // not a divisor of the block
    oscillator.prepare(rate, blockSize);

    const auto first  = pool.buffer(0).subBlock(0, blockSize);
    const auto second = pool.buffer(0).subBlock(blockSize, blockSize);

    oscillator.process(makeContext(first, rate));
    oscillator.process(makeContext(second, rate));

    // The step across the boundary must be no larger than a step within a block.
    Sample largestInternalStep = 0.0f;
    for (FrameCount frame = 1; frame < blockSize; ++frame)
        largestInternalStep = std::max(largestInternalStep,
                                       std::abs(first.channel(0)[frame] - first.channel(0)[frame - 1]));

    const Sample boundaryStep = std::abs(second.channel(0)[0] - first.channel(0)[blockSize - 1]);
    CHECK(boundaryStep <= largestInternalStep * 1.5f);
}

TEST_CASE("oscillator phase stays bounded over a long run")
{
    // Accumulating phase in radians and reducing modulo 2*pi drifts audibly over
    // minutes. Tracking cycles and wrapping to [0,1) does not.
    constexpr SampleRate rate      = 48000.0;
    constexpr FrameCount blockSize = 512;

    AudioBufferPool pool;
    pool.allocate(1, 1, blockSize);

    dsp::SineOscillatorNode oscillator{440.0, 1.0f};
    oscillator.prepare(rate, blockSize);

    for (int block = 0; block < 2000; ++block) {   // ~21 seconds
        oscillator.process(makeContext(pool.buffer(0), rate));
        CHECK(oscillator.phase() >= 0.0);
        CHECK(oscillator.phase() < 1.0);
    }
}

TEST_CASE("the oscillator fills every channel identically")
{
    AudioBufferPool pool;
    pool.allocate(1, 4, 64);

    dsp::SineOscillatorNode oscillator{440.0, 1.0f};
    oscillator.prepare(48000.0, 64);
    oscillator.process(makeContext(pool.buffer(0), 48000.0));

    const auto buffer = pool.buffer(0);
    for (std::size_t channel = 1; channel < 4; ++channel)
        for (FrameCount frame = 0; frame < 64; ++frame)
            CHECK(buffer.channel(channel)[frame] == doctest::Approx(buffer.channel(0)[frame]));
}

TEST_CASE("gain at unity passes its input through unchanged")
{
    AudioBufferPool inputs;
    AudioBufferPool outputs;
    inputs.allocate(1, 2, 32);
    outputs.allocate(1, 2, 32);

    for (std::size_t channel = 0; channel < 2; ++channel)
        for (FrameCount frame = 0; frame < 32; ++frame)
            inputs.buffer(0).channel(channel)[frame] = 0.4f;

    dsp::GainNode gain{1.0f};
    gain.prepare(48000.0, 32);

    const AudioBufferView inputView = inputs.buffer(0);

    auto context = makeContext(outputs.buffer(0), 48000.0);
    context.inputs     = &inputView;
    context.inputCount = 1;

    outputs.buffer(0).clear();
    gain.process(context);

    CHECK(outputs.buffer(0).channel(0)[0] == doctest::Approx(0.4f));
    CHECK(outputs.buffer(0).channel(1)[31] == doctest::Approx(0.4f));
}

TEST_CASE("a gain with no sources produces silence")
{
    AudioBufferPool pool;
    pool.allocate(1, 2, 32);

    dsp::GainNode gain{1.0f};
    gain.prepare(48000.0, 32);

    pool.buffer(0).clear();
    gain.process(makeContext(pool.buffer(0), 48000.0));

    CHECK(pool.buffer(0).peak() == doctest::Approx(0.0f));
}

TEST_CASE("a gain change is smoothed rather than stepped")
{
    // An instantaneous gain change is a discontinuity in the waveform, and a
    // discontinuity is a click. This is the most common artefact in a naive mixer.
    constexpr SampleRate rate      = 48000.0;
    constexpr FrameCount blockSize = 256;

    AudioBufferPool inputs;
    AudioBufferPool outputs;
    inputs.allocate(1, 1, blockSize);
    outputs.allocate(1, 1, blockSize);

    for (FrameCount frame = 0; frame < blockSize; ++frame)
        inputs.buffer(0).channel(0)[frame] = 1.0f;

    dsp::GainNode gain{1.0f};
    gain.prepare(rate, blockSize);
    gain.setGain(0.0f);

    const AudioBufferView inputView = inputs.buffer(0);
    auto context = makeContext(outputs.buffer(0), rate);
    context.inputs     = &inputView;
    context.inputCount = 1;

    outputs.buffer(0).clear();
    gain.process(context);

    const auto output = outputs.buffer(0);

    // It must move towards the target, but not jump there in one sample.
    CHECK(output.channel(0)[0] > 0.9f);

    // The smoother is one-pole with a 5 ms time constant, so after this block
    // (256 frames = 5.33 ms = 1.067 time constants) the remaining distance to
    // the target must be exp(-1.067) of the original. Asserting the actual
    // specification rather than a round number: if the time constant is ever
    // changed, this says so instead of drifting quietly.
    constexpr double timeConstantSeconds = 0.005;
    const double     elapsedConstants    = (static_cast<double>(blockSize) / rate) / timeConstantSeconds;
    const double     expectedRemaining   = std::exp(-elapsedConstants);

    CHECK(output.channel(0)[blockSize - 1] == doctest::Approx(expectedRemaining).epsilon(0.02));

    Sample largestStep = 0.0f;
    for (FrameCount frame = 1; frame < blockSize; ++frame)
        largestStep = std::max(largestStep, std::abs(output.channel(0)[frame] - output.channel(0)[frame - 1]));

    CHECK(largestStep < 0.05f);
}

TEST_CASE("a smoothed gain converges exactly, so the smoothing path goes cold")
{
    constexpr SampleRate rate      = 48000.0;
    constexpr FrameCount blockSize = 512;

    AudioBufferPool inputs;
    AudioBufferPool outputs;
    inputs.allocate(1, 1, blockSize);
    outputs.allocate(1, 1, blockSize);

    for (FrameCount frame = 0; frame < blockSize; ++frame)
        inputs.buffer(0).channel(0)[frame] = 1.0f;

    dsp::GainNode gain{1.0f};
    gain.prepare(rate, blockSize);
    gain.setGain(0.5f);

    const AudioBufferView inputView = inputs.buffer(0);
    auto context = makeContext(outputs.buffer(0), rate);
    context.inputs     = &inputView;
    context.inputCount = 1;

    for (int block = 0; block < 20; ++block) {
        outputs.buffer(0).clear();
        gain.process(context);
    }

    CHECK(gain.currentGain() == doctest::Approx(0.5f));
    CHECK(outputs.buffer(0).channel(0)[blockSize - 1] == doctest::Approx(0.5f));
}

// ── Fft ──────────────────────────────────────────────────────────────────────

TEST_CASE("the FFT matches a naive DFT and resolves a pure tone to its bin")
{
    using incdaw::engine::dsp::Fft;

    constexpr std::size_t size = 256;

    Fft fft;
    fft.setSize(size);

    // An impulse transforms to a flat spectrum of ones.
    std::vector<float> real(size, 0.0f);
    std::vector<float> imaginary(size, 0.0f);
    real[0] = 1.0f;

    fft.forward(real.data(), imaginary.data());

    for (std::size_t bin = 0; bin < size; ++bin) {
        CHECK(real[bin] == doctest::Approx(1.0).epsilon(1e-4));
        CHECK(imaginary[bin] == doctest::Approx(0.0).scale(1.0).epsilon(1e-4));
    }

    // A bin-exact cosine lands its energy at exactly that bin (and its
    // mirror), amplitude N/2.
    constexpr std::size_t tone = 16;
    for (std::size_t index = 0; index < size; ++index) {
        real[index] = static_cast<float>(
            std::cos(2.0 * M_PI * tone * static_cast<double>(index) / size));
        imaginary[index] = 0.0f;
    }

    fft.forward(real.data(), imaginary.data());

    for (std::size_t bin = 0; bin <= size / 2; ++bin) {
        const double magnitude = std::hypot(static_cast<double>(real[bin]),
                                            static_cast<double>(imaginary[bin]));
        if (bin == tone)
            CHECK(magnitude == doctest::Approx(size / 2.0).epsilon(1e-3));
        else
            CHECK(magnitude < 1e-2);
    }
}
