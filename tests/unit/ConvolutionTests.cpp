// The convolution reverb (A11).
//
// A partitioned convolver has one property that settles whether it is correct
// at all: convolving with a unit impulse must hand back the input, sample for
// sample, one partition later. Every partitioning bug — a delay line read the
// wrong way round, an overlap kept instead of discarded, a spectrum mirrored
// wrong — breaks that immediately, and nothing else about the effect is worth
// testing until it holds.
//
// So the tests here feed impulses whose answer is known by hand: a delta, a
// delta at a known offset, and two taps at once.

#include "doctest.h"

#include "engine/audio/WavFile.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/dsp/effects/ConvolutionReverb.h"

#include <cmath>
#include <cstring>
#include <filesystem>
#include <memory>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
using namespace incdaw::engine::dsp;

namespace rt = incdaw::engine::rt;

namespace {

constexpr FrameCount blockSize  = 256;
constexpr double     sampleRate = 48000.0;
constexpr auto       latency    = static_cast<std::size_t>(
    ConvolutionReverbEffect::partitionSize);

using Convolver = ConvolutionReverbEffect;

std::filesystem::path scratchFile(const char* name)
{
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "incdaw-convolution-tests";

    std::error_code failed;
    std::filesystem::create_directories(directory, failed);

    return directory / name;
}

/// Writes a mono impulse response and returns its path.
std::filesystem::path writeImpulse(const char* name, const std::vector<Sample>& taps)
{
    AudioFileData data;
    data.sampleRate   = sampleRate;
    data.channelCount = 1;
    data.frameCount   = static_cast<FrameCount>(taps.size());
    data.channels     = {taps};

    const std::filesystem::path path = scratchFile(name);
    REQUIRE(WavFile::write(path, data));

    return path;
}

std::vector<Sample> processThrough(Node& node, const std::vector<Sample>& input)
{
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

/// A convolver with one impulse loaded, set to hear only the wet path and to
/// leave the tail alone.
std::unique_ptr<ConvolutionReverbEffect> convolverWith(const std::filesystem::path& impulse)
{
    auto effect = std::make_unique<ConvolutionReverbEffect>();
    effect->prepare(sampleRate, blockSize);

    REQUIRE(effect->loadImpulse(impulse));

    effect->setParameter(Convolver::mix, 1.0);
    effect->setParameter(Convolver::decaySeconds, 8.0);
    effect->setParameter(Convolver::dampingHz, 20000.0);
    effect->setParameter(Convolver::lowCutHz, 20.0);
    effect->setParameter(Convolver::width, 1.0);

    return effect;
}

/// The decay envelope's gain on the partition a tap at `offset` lands in.
///
/// The tail shape is applied per PARTITION rather than per sample — that is
/// what makes Decay an ordinary automatable parameter instead of a rebuild —
/// so a reference has to carry the same step.
double partitionGain(std::size_t offset, double decaySeconds)
{
    const auto partition = offset / Convolver::partitionSize;
    const double perPartition = std::exp(
        -3.0 * static_cast<double>(Convolver::partitionSize) / (decaySeconds * sampleRate));

    return std::pow(perPartition, static_cast<double>(partition));
}

std::vector<Sample> noiseSignal(std::size_t frames)
{
    std::vector<Sample> samples(frames, 0.0f);

    std::uint32_t state = 0xC0FFEE11u;
    for (Sample& sample : samples) {
        state  = state * 1664525u + 1013904223u;
        sample = static_cast<Sample>((static_cast<double>(state) / 4294967295.0) * 0.8 - 0.4);
    }

    return samples;
}

} // namespace

TEST_CASE("the convolver is in the catalogue, and reports its partition of latency")
{
    const BuiltinEffectInfo* info = findBuiltinEffect("incdaw.convolver");
    REQUIRE(info != nullptr);

    CHECK(info->parameterCount == 8);
    CHECK(info->presets.count >= 3);

    std::unique_ptr<Node> node = makeBuiltinEffect("incdaw.convolver", sampleRate);
    REQUIRE(node != nullptr);
    CHECK(node->latencyFrames() == static_cast<FrameCount>(Convolver::partitionSize));
}

TEST_CASE("a convolver with no file still has an impulse")
{
    ConvolutionReverbEffect effect;
    effect.prepare(sampleRate, blockSize);

    CHECK(effect.impulsePath().empty());
    CHECK(effect.partitionCount() > 0);
}

TEST_CASE("convolving with a unit impulse hands the signal back, one partition later")
{
    const std::filesystem::path impulse = writeImpulse("delta.wav", {1.0f});

    std::unique_ptr<ConvolutionReverbEffect> effect = convolverWith(impulse);

    const std::vector<Sample> input  = noiseSignal(2048);
    const std::vector<Sample> output = processThrough(*effect, input);

    // Before the latency has elapsed there is nothing.
    for (std::size_t index = 0; index < latency; ++index)
        REQUIRE(std::fabs(static_cast<double>(output[index])) < 1e-6);

    double worst = 0.0;
    for (std::size_t index = latency; index < output.size(); ++index)
        worst = std::max(worst, std::fabs(static_cast<double>(output[index])
                                          - static_cast<double>(input[index - latency])));

    CAPTURE(worst);
    CHECK(worst < 1e-4);
}

TEST_CASE("a tap several partitions in delays by exactly that many samples")
{
    // 700 is well past one partition, so the delay line, its wrap and the
    // partition ordering are all exercised.
    constexpr std::size_t offset = 700;

    std::vector<Sample> taps(offset + 1, 0.0f);
    taps[offset] = 1.0f;

    std::unique_ptr<ConvolutionReverbEffect> effect =
        convolverWith(writeImpulse("offset.wav", taps));

    const std::vector<Sample> input  = noiseSignal(4096);
    const std::vector<Sample> output = processThrough(*effect, input);

    const double gain = partitionGain(offset, 8.0);

    double worst = 0.0;
    for (std::size_t index = latency + offset; index < output.size(); ++index)
        worst = std::max(worst,
                         std::fabs(static_cast<double>(output[index])
                                   - gain * static_cast<double>(input[index - latency - offset])));

    CAPTURE(worst);
    CHECK(worst < 1e-4);
}

TEST_CASE("two taps sum, at their own delays and their own weights")
{
    constexpr std::size_t offset = 400;

    std::vector<Sample> taps(offset + 1, 0.0f);
    taps[0]      = 1.0f;
    taps[offset] = 0.5f;

    std::unique_ptr<ConvolutionReverbEffect> effect =
        convolverWith(writeImpulse("two-taps.wav", taps));

    const std::vector<Sample> input  = noiseSignal(4096);
    const std::vector<Sample> output = processThrough(*effect, input);

    // The impulse is normalised to unit energy so that swapping one for
    // another does not change how loud the send is; the reference has to
    // carry the same scale.
    const double scale = 1.0 / std::sqrt(1.0 * 1.0 + 0.5 * 0.5);

    double worst = 0.0;
    for (std::size_t index = latency + offset; index < output.size(); ++index) {
        const double expected =
            scale * (static_cast<double>(input[index - latency])
                     + 0.5 * partitionGain(offset, 8.0)
                           * static_cast<double>(input[index - latency - offset]));

        worst = std::max(worst, std::fabs(static_cast<double>(output[index]) - expected));
    }

    CAPTURE(worst);
    CHECK(worst < 1e-4);
}

TEST_CASE("reverse plays the impulse backwards")
{
    constexpr std::size_t offset = 300;

    std::vector<Sample> taps(offset + 1, 0.0f);
    taps[0] = 1.0f;   // forwards: no delay. Backwards: a delay of `offset`.

    std::unique_ptr<ConvolutionReverbEffect> effect =
        convolverWith(writeImpulse("reverse.wav", taps));
    effect->setParameter(Convolver::reverse, 1.0);

    const std::vector<Sample> input  = noiseSignal(4096);
    const std::vector<Sample> output = processThrough(*effect, input);

    // Nothing at all until the reversed tap arrives.
    for (std::size_t index = latency; index < latency + offset - 4; ++index)
        REQUIRE(std::fabs(static_cast<double>(output[index])) < 1e-4);

    const double gain = partitionGain(offset, 8.0);

    double worst = 0.0;
    for (std::size_t index = latency + offset; index < output.size(); ++index)
        worst = std::max(worst,
                         std::fabs(static_cast<double>(output[index])
                                   - gain * static_cast<double>(input[index - latency - offset])));

    CHECK(worst < 1e-4);
}

TEST_CASE("decay shortens the tail without touching its head")
{
    // A long flat impulse, so the envelope is the only thing shaping it.
    std::vector<Sample> taps(4096, 0.0f);
    for (std::size_t index = 0; index < taps.size(); index += 128)
        taps[index] = 1.0f;

    const std::filesystem::path path = writeImpulse("flat.wav", taps);

    const auto tailEnergyWith = [&path](double decay) {
        std::unique_ptr<ConvolutionReverbEffect> effect = convolverWith(path);
        effect->setParameter(Convolver::decaySeconds, decay);

        std::vector<Sample> click(8192, 0.0f);
        click[0] = 1.0f;

        const std::vector<Sample> output = processThrough(*effect, click);

        double energy = 0.0;
        for (std::size_t index = 3000; index < output.size(); ++index)
            energy += static_cast<double>(output[index]) * static_cast<double>(output[index]);

        return energy;
    };

    CHECK(tailEnergyWith(0.1) < tailEnergyWith(8.0) * 0.5);
}

TEST_CASE("the impulse's path rides in the state, and comes back with it")
{
    const std::filesystem::path impulse = writeImpulse("state.wav", {1.0f, 0.25f});

    ConvolutionReverbEffect saved;
    saved.prepare(sampleRate, blockSize);
    REQUIRE(saved.loadImpulse(impulse));
    saved.setParameter(Convolver::mix, 0.42);

    std::vector<std::uint8_t> blob;
    REQUIRE(saved.saveState(blob));

    ConvolutionReverbEffect restored;
    restored.prepare(sampleRate, blockSize);
    REQUIRE(restored.loadState(blob.data(), blob.size()));

    CHECK(restored.impulsePath() == impulse.string());
    CHECK(restored.value(Convolver::mix) == doctest::Approx(0.42));
    CHECK(restored.partitionCount() == saved.partitionCount());
}

TEST_CASE("a state blob from before the string section still loads")
{
    // Version 1: header, count, pairs, and nothing after them.
    const std::vector<std::pair<std::uint32_t, double>> values = {{Convolver::mix, 0.3}};

    std::vector<std::uint8_t> blob;
    const auto appendU32 = [&blob](std::uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8)
            blob.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFu));
    };

    appendU32(1);   // version
    appendU32(static_cast<std::uint32_t>(values.size()));

    for (const auto& [id, value] : values) {
        appendU32(id);

        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        for (int shift = 0; shift < 64; shift += 8)
            blob.push_back(static_cast<std::uint8_t>((bits >> shift) & 0xFFu));
    }

    ConvolutionReverbEffect effect;
    effect.prepare(sampleRate, blockSize);

    REQUIRE(effect.loadState(blob.data(), blob.size()));
    CHECK(effect.value(Convolver::mix) == doctest::Approx(0.3));
    CHECK(effect.impulsePath().empty());
}

TEST_CASE("a missing impulse plays the generated one rather than going silent")
{
    ConvolutionReverbEffect effect;
    effect.prepare(sampleRate, blockSize);

    effect.applyStateString(Convolver::impulseKey, "/nowhere/at/all.wav");

    // The path is kept, so reconnecting the drive restores the session...
    CHECK(effect.impulsePath() == "/nowhere/at/all.wav");

    // ...and meanwhile the send is not a hole in the mix.
    CHECK(effect.partitionCount() > 0);
}

TEST_CASE("rendering allocates nothing on the audio thread")
{
    std::unique_ptr<ConvolutionReverbEffect> effect =
        convolverWith(writeImpulse("realtime.wav", std::vector<Sample>(2048, 0.01f)));

    AudioBufferPool pool;
    pool.allocate(2, 2, blockSize);

    rt::resetViolations();

    {
        const rt::ScopedRealtimeContext scope;

        for (int block = 0; block < 8; ++block) {
            const AudioBufferView inputView  = pool.buffer(0);
            const AudioBufferView outputView = pool.buffer(1);

            inputView.clear();
            outputView.clear();

            for (FrameCount frame = 0; frame < blockSize; ++frame)
                inputView.channel(0)[frame] = 0.1f;

            const AudioBufferView inputs[] = {inputView};

            ProcessContext context{};
            context.output     = outputView;
            context.inputs     = inputs;
            context.inputCount = 1;
            context.frameCount = blockSize;

            effect->process(context);
            effect->setParameter(Convolver::decaySeconds, 1.0 + 0.5 * block);
        }
    }

    CHECK(rt::allocationViolations() == 0);
}
