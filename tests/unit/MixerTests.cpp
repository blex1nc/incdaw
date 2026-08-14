// Phase 10 — the mixer, and delay compensation that actually compensates.
//
// The load-bearing test is the roadmap's exit criterion: a path carrying
// artificial latency must stay phase-aligned with an uncompensated parallel
// path. It is written so that it fails when compensation is switched off — a
// PDC test that passes either way is testing nothing.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/LevelMeter.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/DelayLineNode.h"
#include "engine/dsp/MixerStripNode.h"
#include "engine/graph/RenderGraph.h"

#include <chrono>
#include <cmath>
#include <memory>
#include <vector>

using namespace incdaw::engine;

namespace {

/// Emits a single full-scale sample at `position`, then silence — the signal
/// that makes alignment visible.
class ImpulseNode final : public Node {
public:
    explicit ImpulseNode(FramePosition position, FrameCount latency = 0) noexcept
        : position_(position), latency_(latency) {}

    void process(const ProcessContext& context) noexcept override
    {
        const FramePosition first = context.playPosition;
        const FramePosition last  = first + context.frameCount;

        if (position_ < first || position_ >= last)
            return;

        const auto offset = static_cast<FrameCount>(position_ - first);

        for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel)
            context.output.channel(channel)[offset] = Sample{1};
    }

    /// Claims a latency it does not actually apply — a stand-in for a plugin
    /// that reports one. The graph must compensate the *other* path by this
    /// much, which is exactly what a real latent plugin needs.
    [[nodiscard]] FrameCount latencyFrames() const noexcept override { return latency_; }
    [[nodiscard]] const char* name() const noexcept override { return "Impulse"; }

private:
    FramePosition position_;
    FrameCount    latency_;
};

/// Applies a real delay AND reports it, the way a latent processor behaves.
class LatentProcessorNode final : public Node {
public:
    explicit LatentProcessorNode(FrameCount latency) noexcept : delay_(latency), latency_(latency) {}

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override
    {
        delay_.prepare(sampleRate, maxBlockSize);
    }

    void process(const ProcessContext& context) noexcept override { delay_.process(context); }

    [[nodiscard]] FrameCount latencyFrames() const noexcept override { return latency_; }
    [[nodiscard]] const char* name() const noexcept override { return "Latent"; }

private:
    dsp::DelayLineNode delay_;
    FrameCount         latency_;
};

struct TestOutput {
    TestOutput(std::size_t channels, FrameCount frames) { pool.allocate(1, channels, frames); }

    [[nodiscard]] AudioBufferView view() const noexcept { return pool.buffer(0); }

    AudioBufferPool pool;
};

/// Renders `blocks` blocks and returns channel 0, concatenated.
std::vector<Sample> render(CompiledGraph& graph, FrameCount blockSize, int blocks,
                           std::size_t channels = 2)
{
    TestOutput output{channels, blockSize};
    std::vector<Sample> samples;

    for (int block = 0; block < blocks; ++block) {
        output.view().clear();
        graph.process(output.view(), blockSize, static_cast<FramePosition>(block) * blockSize);

        const Sample* channel = output.view().channel(0);
        for (FrameCount frame = 0; frame < blockSize; ++frame)
            samples.push_back(channel[frame]);
    }

    return samples;
}

/// Positions at which a signal is meaningfully non-zero.
std::vector<std::size_t> onsets(const std::vector<Sample>& samples, Sample threshold = 0.01f)
{
    std::vector<std::size_t> found;
    for (std::size_t index = 0; index < samples.size(); ++index)
        if (std::abs(samples[index]) > threshold)
            found.push_back(index);

    return found;
}

/// The graph the exit criterion describes: one impulse feeding two paths into a
/// summing node, one of which carries latency.
std::unique_ptr<CompiledGraph> buildParallelPaths(FrameCount latency, bool compensate,
                                                  FrameCount blockSize = 64)
{
    GraphBuilder builder;
    builder.setDelayCompensationEnabled(compensate);

    const auto source    = builder.addNode(std::make_unique<ImpulseNode>(0));
    const auto latent    = builder.addNode(std::make_unique<LatentProcessorNode>(latency));
    const auto latentStrip = builder.addNode(std::make_unique<dsp::MixerStripNode>());
    const auto direct    = builder.addNode(std::make_unique<dsp::MixerStripNode>());
    const auto summing   = builder.addNode(std::make_unique<dsp::MixerStripNode>());

    // Both paths run through a strip, so the two arrivals carry identical gain
    // and the only thing the test can be measuring is their alignment.
    builder.connect(source, latent);
    builder.connect(latent, latentStrip);
    builder.connect(latentStrip, summing);
    builder.connect(source, direct);
    builder.connect(direct, summing);

    builder.setMaster(summing);

    return builder.compile(48000.0, blockSize, 2);
}

} // namespace

// ── The exit criterion ────────────────────────────────────────────────────────

TEST_CASE("a latent path stays phase-aligned with an uncompensated parallel path")
{
    constexpr FrameCount latency   = 100;
    constexpr FrameCount blockSize = 64;

    auto compensated = buildParallelPaths(latency, true, blockSize);
    REQUIRE(compensated != nullptr);

    const std::vector<Sample> aligned = render(*compensated, blockSize, 8);
    const std::vector<std::size_t> alignedOnsets = onsets(aligned);

    // One impulse, not two: both paths arrived at the same sample and summed.
    REQUIRE(alignedOnsets.size() == 1);
    CHECK(alignedOnsets.front() == static_cast<std::size_t>(latency));
    // Two centre-panned paths (-3 dB each) summed and passed through a third:
    // (0.7071 + 0.7071) * 0.7071 = 1.
    CHECK(aligned[alignedOnsets.front()] == doctest::Approx(1.0).epsilon(0.01));

    // The graph reports the delay it imposed, which is what the recording path
    // and the UI need in order to stay honest about it.
    CHECK(compensated->latencyFrames() == latency);

    // And the same graph without compensation smears into two impulses. If this
    // ever stops failing, the test above has stopped proving anything.
    auto uncompensated = buildParallelPaths(latency, false, blockSize);
    REQUIRE(uncompensated != nullptr);

    const std::vector<std::size_t> smeared = onsets(render(*uncompensated, blockSize, 8));
    REQUIRE(smeared.size() == 2);
    CHECK(smeared[0] == 0);
    CHECK(smeared[1] == static_cast<std::size_t>(latency));
}

TEST_CASE("compensation handles latency that is not a multiple of the block size")
{
    for (const FrameCount latency : {1, 63, 64, 65, 127, 500}) {
        auto graph = buildParallelPaths(latency, true, 64);
        REQUIRE(graph != nullptr);

        const std::vector<std::size_t> found = onsets(render(*graph, 64, 16));

        INFO("latency = " << latency);
        REQUIRE(found.size() == 1);
        CHECK(found.front() == static_cast<std::size_t>(latency));
    }
}

TEST_CASE("a graph with no latency gains no delay lines")
{
    GraphBuilder builder;

    const auto source  = builder.addNode(std::make_unique<ImpulseNode>(0));
    const auto strip   = builder.addNode(std::make_unique<dsp::MixerStripNode>());
    const auto master  = builder.addNode(std::make_unique<dsp::MixerStripNode>());

    builder.connect(source, strip);
    builder.connect(strip, master);
    builder.setMaster(master);

    auto graph = builder.compile(48000.0, 64, 2);
    REQUIRE(graph != nullptr);

    // Compensation must be free when nothing needs compensating.
    CHECK(graph->nodeCount() == 3);
    CHECK(graph->latencyFrames() == 0);
}

TEST_CASE("compensation reaches through a chain, not just the last edge")
{
    // source -> latent -> strip ─┐
    //        └──────────────────> summing
    // The compensating delay must be sized from the *accumulated* latency, not
    // from the latent node's own edge.
    GraphBuilder builder;

    const auto source  = builder.addNode(std::make_unique<ImpulseNode>(0));
    const auto latent  = builder.addNode(std::make_unique<LatentProcessorNode>(48));
    const auto middle  = builder.addNode(std::make_unique<dsp::MixerStripNode>());
    const auto summing = builder.addNode(std::make_unique<dsp::MixerStripNode>());

    builder.connect(source, latent);
    builder.connect(latent, middle);
    builder.connect(middle, summing);
    builder.connect(source, summing);

    builder.setMaster(summing);

    auto graph = builder.compile(48000.0, 32, 2);
    REQUIRE(graph != nullptr);

    const std::vector<std::size_t> found = onsets(render(*graph, 32, 8));
    REQUIRE(found.size() == 1);
    CHECK(found.front() == 48);
}

// ── The delay line itself ─────────────────────────────────────────────────────

TEST_CASE("a delay line is sample-accurate across block boundaries")
{
    constexpr FrameCount delay     = 70;
    constexpr FrameCount blockSize = 32;

    GraphBuilder builder;
    const auto source = builder.addNode(std::make_unique<ImpulseNode>(5));
    const auto line   = builder.addNode(std::make_unique<dsp::DelayLineNode>(delay));

    builder.connect(source, line);
    builder.setMaster(line);

    auto graph = builder.compile(48000.0, blockSize, 2);
    REQUIRE(graph != nullptr);

    const std::vector<std::size_t> found = onsets(render(*graph, blockSize, 8));
    REQUIRE(found.size() == 1);
    CHECK(found.front() == 5 + delay);
    CHECK(graph->latencyFrames() == delay);
}

TEST_CASE("a zero-length delay line is a pass-through")
{
    GraphBuilder builder;
    const auto source = builder.addNode(std::make_unique<ImpulseNode>(3));
    const auto line   = builder.addNode(std::make_unique<dsp::DelayLineNode>(0));

    builder.connect(source, line);
    builder.setMaster(line);

    auto graph = builder.compile(48000.0, 16, 2);
    REQUIRE(graph != nullptr);

    const std::vector<std::size_t> found = onsets(render(*graph, 16, 4));
    REQUIRE(found.size() == 1);
    CHECK(found.front() == 3);
}

// ── The mixer strip ───────────────────────────────────────────────────────────

TEST_CASE("the pan law is constant power")
{
    Sample left = 0.0f;
    Sample right = 0.0f;

    dsp::MixerStripNode::panGains(0.0, left, right);
    CHECK(left == doctest::Approx(0.7071).epsilon(0.001));
    CHECK(right == doctest::Approx(0.7071).epsilon(0.001));

    dsp::MixerStripNode::panGains(-1.0, left, right);
    CHECK(left == doctest::Approx(1.0).epsilon(0.001));
    CHECK(right == doctest::Approx(0.0).epsilon(0.001));

    dsp::MixerStripNode::panGains(1.0, left, right);
    CHECK(left == doctest::Approx(0.0).epsilon(0.001));
    CHECK(right == doctest::Approx(1.0).epsilon(0.001));

    // Power is conserved wherever the source sits, which is the whole point:
    // sweeping across the image must not change how loud something is.
    for (const double pan : {-0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75}) {
        dsp::MixerStripNode::panGains(pan, left, right);
        CHECK(left * left + right * right == doctest::Approx(1.0).epsilon(0.001));
    }

    // Out-of-range values are clamped rather than producing a gain above one.
    dsp::MixerStripNode::panGains(-4.0, left, right);
    CHECK(left == doctest::Approx(1.0).epsilon(0.001));
}

TEST_CASE("a strip sums, pans, mutes and inverts")
{
    GraphBuilder builder;

    const auto first  = builder.addNode(std::make_unique<ImpulseNode>(0));
    const auto second = builder.addNode(std::make_unique<ImpulseNode>(0));

    auto stripNode = std::make_unique<dsp::MixerStripNode>();
    dsp::MixerStripNode* strip = stripNode.get();
    const auto stripIndex = builder.addNode(std::move(stripNode));

    builder.connect(first, stripIndex);
    builder.connect(second, stripIndex);
    builder.setMaster(stripIndex);

    strip->setPan(-1.0);   // hard left, so the two channels differ measurably

    auto graph = builder.compile(48000.0, 16, 2);
    REQUIRE(graph != nullptr);

    TestOutput output{2, 16};
    output.view().clear();
    graph->process(output.view(), 16, 0);

    // Two impulses summed, then panned hard left.
    CHECK(output.view().channel(0)[0] == doctest::Approx(2.0).epsilon(0.01));
    CHECK(output.view().channel(1)[0] == doctest::Approx(0.0).epsilon(0.01));

    // Every parameter change ramps, so each subcase runs the graph past the
    // impulse until the ramp has arrived, then renders the impulse again. The
    // blocks in between carry no signal — which is exactly why the ramp has to
    // be given time rather than assumed instant.
    const auto settle = [&graph, &output] {
        for (int block = 1; block < 200; ++block) {
            output.view().clear();
            graph->process(output.view(), 16, static_cast<FramePosition>(block) * 16);
        }

        output.view().clear();
        graph->process(output.view(), 16, 0);
    };

    SUBCASE("mute silences it")
    {
        strip->setMuted(true);
        settle();

        CHECK(std::abs(output.view().channel(0)[0]) < 0.001f);
    }

    SUBCASE("polarity inverts without changing level")
    {
        strip->setPolarityInverted(true);
        settle();

        CHECK(output.view().channel(0)[0] == doctest::Approx(-2.0).epsilon(0.01));
    }
}

TEST_CASE("a fader change is ramped, not stepped")
{
    GraphBuilder builder;

    const auto source = builder.addNode(std::make_unique<ImpulseNode>(0));

    auto stripNode = std::make_unique<dsp::MixerStripNode>();
    dsp::MixerStripNode* strip = stripNode.get();
    const auto stripIndex = builder.addNode(std::move(stripNode));

    builder.connect(source, stripIndex);
    builder.setMaster(stripIndex);

    auto graph = builder.compile(48000.0, 64, 2);
    REQUIRE(graph != nullptr);

    strip->setGain(0.0f);

    TestOutput output{2, 64};
    output.view().clear();
    graph->process(output.view(), 64, 0);

    // Mid-ramp: the gain has started falling but has not arrived, which is
    // precisely what stops a fader move from clicking.
    const Sample first = std::abs(output.view().channel(0)[0]);
    CHECK(first > 0.0f);
    CHECK(first < 0.7071f);
}

TEST_CASE("metering reports peak and RMS of a known signal")
{
    LevelMeter meter;
    meter.prepare(48000.0);

    AudioBufferPool pool;
    pool.allocate(1, 1, 4800);
    const AudioBufferView buffer = pool.buffer(0);

    // Full-scale sine: peak 1, RMS 1/sqrt(2).
    Sample* samples = buffer.channel(0);
    for (FrameCount frame = 0; frame < 4800; ++frame)
        samples[frame] = static_cast<Sample>(std::sin(2.0 * 3.14159265358979323846
                                                      * 100.0 * static_cast<double>(frame) / 48000.0));

    // A second of audio, so the 300 ms window is fully charged.
    for (int block = 0; block < 10; ++block)
        meter.measure(buffer, 4800);

    CHECK(meter.peak() == doctest::Approx(1.0).epsilon(0.05));
    CHECK(meter.rms() == doctest::Approx(0.7071).epsilon(0.05));

    SUBCASE("silence decays the peak instead of holding it forever")
    {
        buffer.clear();

        for (int block = 0; block < 100; ++block)
            meter.measure(buffer, 4800);

        CHECK(meter.peak() < 0.1f);
        CHECK(meter.rms() < 0.01f);
    }
}

// ── Realtime safety ───────────────────────────────────────────────────────────

TEST_CASE("the mixer's nodes allocate nothing while rendering")
{
    auto graph = buildParallelPaths(128, true, 64);
    REQUIRE(graph != nullptr);

    TestOutput output{2, 64};

    rt::resetViolations();

    {
        const rt::ScopedRealtimeContext scope;

        for (int block = 0; block < 16; ++block) {
            output.view().clear();
            graph->process(output.view(), 64, static_cast<FramePosition>(block) * 64);
        }
    }

    // The delay lines compensation inserted allocate in `prepare`, which ran at
    // compile time on this thread — not here.
    CHECK(rt::allocationViolations() == 0);
}

TEST_CASE("a mixer-sized graph renders far inside its block budget")
{
    // 64 strips into a master, which is a busy session's mixer.
    GraphBuilder builder;
    const auto master = builder.addNode(std::make_unique<dsp::MixerStripNode>());

    for (int index = 0; index < 64; ++index) {
        const auto source = builder.addNode(std::make_unique<ImpulseNode>(index));
        const auto strip  = builder.addNode(std::make_unique<dsp::MixerStripNode>());

        builder.connect(source, strip);
        builder.connect(strip, master);
    }

    builder.setMaster(master);

    auto graph = builder.compile(48000.0, 256, 2);
    REQUIRE(graph != nullptr);

    TestOutput output{2, 256};

    const auto started = std::chrono::steady_clock::now();

    constexpr int blocks = 200;
    for (int block = 0; block < blocks; ++block) {
        output.view().clear();
        graph->process(output.view(), 256, static_cast<FramePosition>(block) * 256);
    }

    const auto elapsed = std::chrono::steady_clock::now() - started;
    const double perBlock = std::chrono::duration<double, std::milli>(elapsed).count() / blocks;

    // 256 frames at 48 kHz is a 5.33 ms budget.
    MESSAGE("64-strip mixer: " << perBlock << " ms per 256-frame block");
    CHECK(perBlock < 5.33);
}
