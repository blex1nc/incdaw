#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/ChannelStripNode.h"
#include "engine/dsp/DelayLineNode.h"
#include "engine/dsp/GainNode.h"
#include "engine/graph/RenderGraph.h"
#include "project/GraphCompiler.h"

#include <cmath>
#include <memory>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
using project::EntityId;
using project::Project;

namespace {

/// Emits a single full-scale sample on the first frame of the first block, then
/// silence. An impulse is what makes an alignment error visible: two paths that
/// agree produce one spike, two that do not produce two.
class ImpulseNode final : public Node {
public:
    void prepare(SampleRate, FrameCount) override { fired_ = false; }

    void process(const ProcessContext& context) noexcept override
    {
        if (fired_ || context.frameCount <= 0)
            return;

        fired_ = true;

        for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel)
            context.output.channel(channel)[0] = Sample{1};
    }

    [[nodiscard]] const char* name() const noexcept override { return "Impulse"; }

private:
    bool fired_ = false;
};

/// Frames at which the output rises above silence, across several blocks.
std::vector<FramePosition> onsetsOf(CompiledGraph& graph, FrameCount block, int blocks)
{
    AudioBufferPool pool;
    pool.allocate(1, 2, block);

    std::vector<FramePosition> onsets;

    for (int index = 0; index < blocks; ++index) {
        const auto position = static_cast<FramePosition>(index) * block;
        graph.process(pool.buffer(0), block, position);

        const Sample* samples = pool.buffer(0).channel(0);

        for (FrameCount frame = 0; frame < block; ++frame)
            if (std::abs(samples[frame]) > 1e-4f)
                onsets.push_back(position + frame);
    }

    return onsets;
}

} // namespace

// ── Delay line ────────────────────────────────────────────────────────────────

TEST_CASE("a delay line delays by exactly what it reports")
{
    constexpr FrameCount block = 64;
    constexpr FrameCount delay = 100;

    GraphBuilder builder;
    const auto impulse = builder.addNode(std::make_unique<ImpulseNode>());
    const auto line    = builder.addNode(std::make_unique<dsp::DelayLineNode>(delay, 2));

    builder.connect(impulse, line);
    builder.setMaster(line);

    const auto graph = builder.compile(48000.0, block, 2);
    REQUIRE(graph != nullptr);
    CHECK(graph->latencyFrames() == delay);

    const auto onsets = onsetsOf(*graph, block, 4);

    REQUIRE(onsets.size() == 1);
    CHECK(onsets[0] == delay);
}

TEST_CASE("a delay line emits silence before it has anything to emit")
{
    constexpr FrameCount block = 32;

    GraphBuilder builder;
    const auto line = builder.addNode(std::make_unique<dsp::DelayLineNode>(64, 2));
    builder.setMaster(line);

    const auto graph = builder.compile(48000.0, block, 2);
    REQUIRE(graph != nullptr);

    AudioBufferPool pool;
    pool.allocate(1, 2, block);

    // Uninitialised ring memory would come out as a click on the first blocks.
    for (int index = 0; index < 4; ++index) {
        graph->process(pool.buffer(0), block, static_cast<FramePosition>(index) * block);
        CHECK(pool.buffer(0).peak() == doctest::Approx(0.0));
    }
}

// ── Phase 10 exit criterion ───────────────────────────────────────────────────

TEST_CASE("a latent chain stays phase-aligned with an uncompensated parallel path")
{
    // docs/ROADMAP.md Phase 10: PDC test — a chain with artificial latency stays
    // phase-aligned with a parallel path that has none.
    constexpr FrameCount block   = 64;
    constexpr FrameCount latency = 128;

    const auto build = [&](bool compensate) {
        GraphBuilder builder;
        builder.setDelayCompensationEnabled(compensate);

        // One source, two paths, one summing point. The upper path runs through
        // something that reports 128 frames of latency — a look-ahead limiter, a
        // linear-phase EQ, most plugins.
        const auto impulse = builder.addNode(std::make_unique<ImpulseNode>());
        const auto latent  = builder.addNode(std::make_unique<dsp::DelayLineNode>(latency, 2));
        const auto dry     = builder.addNode(std::make_unique<dsp::GainNode>(1.0f));
        const auto bus     = builder.addNode(std::make_unique<dsp::GainNode>(1.0f));

        builder.connect(impulse, latent);
        builder.connect(latent, bus);
        builder.connect(impulse, dry);
        builder.connect(dry, bus);
        builder.setMaster(bus);

        struct Built {
            std::unique_ptr<CompiledGraph> graph;
            std::size_t                    inserted = 0;
        };

        Built built;
        built.graph    = builder.compile(48000.0, block, 2);
        built.inserted = builder.compensationNodesInserted();
        return built;
    };

    SUBCASE("without compensation the paths arrive apart")
    {
        auto built = build(false);
        REQUIRE(built.graph != nullptr);
        CHECK(built.inserted == 0);

        const auto onsets = onsetsOf(*built.graph, block, 8);

        // Two spikes: the dry path immediately, the latent one 128 frames later.
        // That difference is comb filtering, not a mix.
        REQUIRE(onsets.size() == 2);
        CHECK(onsets[0] == 0);
        CHECK(onsets[1] == latency);
    }

    SUBCASE("with compensation they arrive together")
    {
        auto built = build(true);
        REQUIRE(built.graph != nullptr);
        CHECK(built.inserted == 1);              // one delay, on the dry path

        const auto onsets = onsetsOf(*built.graph, block, 8);

        REQUIRE(onsets.size() == 1);             // one spike: the paths agree
        CHECK(onsets[0] == latency);

        // And the graph reports what it costs, so the device and the recording
        // path can account for it.
        CHECK(built.graph->latencyFrames() == latency);
    }
}

TEST_CASE("compensation handles paths of different depths meeting at one node")
{
    constexpr FrameCount block = 64;

    GraphBuilder builder;

    const auto impulse = builder.addNode(std::make_unique<ImpulseNode>());
    const auto slow    = builder.addNode(std::make_unique<dsp::DelayLineNode>(256, 2));
    const auto medium  = builder.addNode(std::make_unique<dsp::DelayLineNode>(64, 2));
    const auto dry     = builder.addNode(std::make_unique<dsp::GainNode>(1.0f));
    const auto bus     = builder.addNode(std::make_unique<dsp::GainNode>(1.0f));

    builder.connect(impulse, slow);
    builder.connect(slow, bus);
    builder.connect(impulse, medium);
    builder.connect(medium, bus);
    builder.connect(impulse, dry);
    builder.connect(dry, bus);
    builder.setMaster(bus);

    const auto graph = builder.compile(48000.0, block, 2);
    REQUIRE(graph != nullptr);
    CHECK(builder.compensationNodesInserted() == 2);

    const auto onsets = onsetsOf(*graph, block, 12);

    REQUIRE(onsets.size() == 1);
    CHECK(onsets[0] == 256);
    CHECK(graph->latencyFrames() == 256);
}

TEST_CASE("a graph with no latency anywhere gains no delay lines")
{
    GraphBuilder builder;

    const auto impulse = builder.addNode(std::make_unique<ImpulseNode>());
    const auto gain    = builder.addNode(std::make_unique<dsp::GainNode>(1.0f));
    const auto bus     = builder.addNode(std::make_unique<dsp::GainNode>(1.0f));

    builder.connect(impulse, gain);
    builder.connect(gain, bus);
    builder.connect(impulse, bus);
    builder.setMaster(bus);

    const auto graph = builder.compile(48000.0, 64, 2);
    REQUIRE(graph != nullptr);

    CHECK(builder.compensationNodesInserted() == 0);
    CHECK(graph->nodeCount() == 3);      // compensation must not add nodes for nothing
    CHECK(graph->latencyFrames() == 0);
}

// ── Strip ─────────────────────────────────────────────────────────────────────

TEST_CASE("polarity inverts and metering reports peak and RMS")
{
    constexpr FrameCount block = 64;

    dsp::ChannelStripNode strip;
    strip.prepare(48000.0, block);

    AudioBufferPool pool;
    pool.allocate(2, 2, block);

    const auto input = pool.buffer(1);
    for (std::size_t channel = 0; channel < 2; ++channel)
        for (FrameCount frame = 0; frame < block; ++frame)
            input.channel(channel)[frame] = 0.5f;

    const auto render = [&](bool flipped) {
        strip.setPolarityFlipped(flipped);
        strip.setPan(0.0f);
        strip.prepare(48000.0, block);

        const auto output = pool.buffer(0);
        output.clear();

        ProcessContext context;
        context.output     = output;
        context.inputs     = &input;
        context.inputCount = 1;
        context.frameCount = block;
        context.sampleRate = 48000.0;

        strip.process(context);
        return output.channel(0)[10];
    };

    const Sample upright = render(false);
    const Sample flipped = render(true);

    CHECK(upright > 0.0f);
    CHECK(flipped == doctest::Approx(-upright));

    // A constant signal has RMS equal to its magnitude, which is what makes it
    // the right test case: any windowing mistake shows up immediately.
    CHECK(strip.lastPeak() == doctest::Approx(std::abs(upright)).epsilon(0.01));
    CHECK(strip.lastRms() == doctest::Approx(std::abs(upright)).epsilon(0.01));
}

// ── Project routing ───────────────────────────────────────────────────────────

TEST_CASE("a channel reaches the master through its mixer node")
{
    Project project;

    project::MixerNode& bus = project.addMixerNode(project::MixerNodeType::bus, "Drums");
    const EntityId busId = bus.id;

    const EntityId channel = project.addChannel("Kick").id;
    project.findChannelForEdit(channel)->outputMixerNode = busId;

    const TempoMap tempoMap{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 128;

    const auto compiled = project::compileProjectGraph(project, tempoMap, options);

    REQUIRE(compiled.graph != nullptr);
    CHECK(compiled.error.empty());

    // master + bus, each an input node and a strip.
    REQUIRE(compiled.mixerOrder.size() == 2);
    CHECK(compiled.mixerOrder[0] == project.masterMixerNode());
    CHECK(compiled.mixerOrder[1] == busId);
    CHECK(compiled.mixerStripNodes.size() == 2);
}

TEST_CASE("a bus fader changes what the master hears")
{
    constexpr FrameCount block = 128;

    Project project;
    const EntityId bus     = project.addMixerNode(project::MixerNodeType::bus, "Drums").id;
    const EntityId channel = project.addChannel("Lead").id;
    project.findChannelForEdit(channel)->outputMixerNode = bus;

    project::Pattern& pattern = project.addPattern("Riff");
    project::MidiEvent note;
    note.type      = project::MidiEventType::note;
    note.duration  = ticksPerQuarterNote;
    note.key       = 60;
    note.value     = 120;
    note.channelId = channel;
    pattern.events.push_back(note);

    const TempoMap tempoMap{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.activePattern = pattern.id;
    options.maxBlockSize  = block;
    options.masterGain    = 1.0f;

    const auto measure = [&] {
        const auto compiled = project::compileProjectGraph(project, tempoMap, options);
        REQUIRE(compiled.graph != nullptr);

        AudioBufferPool pool;
        pool.allocate(1, 2, block);
        compiled.graph->process(pool.buffer(0), block, 0);

        return pool.buffer(0).peak();
    };

    const Sample full = measure();
    CHECK(full > 0.001f);

    project.findMixerNodeForEdit(bus)->volume = 0.25;
    const Sample quiet = measure();

    CHECK(quiet < full);
    CHECK(quiet > 0.0f);

    project.findMixerNodeForEdit(bus)->muted = true;
    CHECK(measure() == doctest::Approx(0.0));
}

TEST_CASE("a routing cycle is rejected and reported, not silently broken")
{
    Project project;
    const EntityId first  = project.addMixerNode(project::MixerNodeType::bus, "A").id;
    const EntityId second = project.addMixerNode(project::MixerNodeType::bus, "B").id;

    project.connect(first, second);
    project.connect(second, first);

    const TempoMap tempoMap{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 64;

    const auto compiled = project::compileProjectGraph(project, tempoMap, options);

    CHECK(compiled.graph == nullptr);
    CHECK(compiled.error == "the connections contain a cycle");
    CHECK(compiled.mixerOrder.empty());
}

TEST_CASE("a send is an extra path, not a replacement for the main one")
{
    Project project;
    const EntityId track  = project.addMixerNode(project::MixerNodeType::track, "Track").id;
    const EntityId reverb = project.addMixerNode(project::MixerNodeType::bus, "Reverb").id;

    project::RoutingConnection& send = project.connect(track, reverb);
    send.isSend = true;
    send.gain   = 0.5;

    const TempoMap tempoMap{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 64;

    const auto compiled = project::compileProjectGraph(project, tempoMap, options);
    REQUIRE(compiled.graph != nullptr);

    // master, track and reverb strips (two nodes each), the send's gain node,
    // and the master gain. The track still reaches the master directly: a send
    // must not steal the signal it copies.
    CHECK(compiled.graph->nodeCount() == 8);
    CHECK(compiled.mixerOrder.size() == 3);
}

TEST_CASE("a channel whose mixer node was deleted still reaches the master")
{
    Project project;
    const EntityId channel = project.addChannel("Orphan").id;
    project.findChannelForEdit(channel)->outputMixerNode = EntityId{9999};

    const TempoMap tempoMap{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 64;

    const auto compiled = project::compileProjectGraph(project, tempoMap, options);

    // Silence would be data loss the user cannot see: a channel that plays
    // nothing looks exactly like a channel with nothing on it.
    REQUIRE(compiled.graph != nullptr);
    CHECK(compiled.channelOrder.size() == 1);
}

// ── Realtime safety ───────────────────────────────────────────────────────────

TEST_CASE("the compensated mixer graph allocates nothing while rendering")
{
    // Delay compensation adds nodes with ring buffers. A ring that grew on the
    // audio thread would be a dropout at the worst possible moment — the first
    // block after a plugin reports its latency.
    constexpr FrameCount block = 128;

    Project project;
    const EntityId bus     = project.addMixerNode(project::MixerNodeType::bus, "Drums").id;
    const EntityId channel = project.addChannel("Kick").id;
    project.findChannelForEdit(channel)->outputMixerNode = bus;

    project::RoutingConnection& send = project.connect(bus, project.masterMixerNode());
    send.isSend = true;
    send.gain   = 0.4;

    project::Pattern& pattern = project.addPattern("Beat");
    project::MidiEvent note;
    note.type      = project::MidiEventType::note;
    note.duration  = ticksPerQuarterNote;
    note.key       = 36;
    note.value     = 110;
    note.channelId = channel;
    pattern.events.push_back(note);

    const TempoMap tempoMap{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.activePattern = pattern.id;
    options.maxBlockSize  = block;

    const auto compiled = project::compileProjectGraph(project, tempoMap, options);
    REQUIRE(compiled.graph != nullptr);

    AudioBufferPool pool;
    pool.allocate(1, 2, block);

    const auto before = rt::allocationViolations();

    {
        const rt::ScopedRealtimeContext scope;

        for (int index = 0; index < 16; ++index)
            compiled.graph->process(pool.buffer(0), block,
                                    static_cast<FramePosition>(index) * block);
    }

    CHECK(rt::allocationViolations() == before);
    CHECK_FALSE(pool.buffer(0).hasNonFiniteSamples());
}
