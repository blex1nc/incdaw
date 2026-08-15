// Phase 13 — hosted plugin latency joins delay compensation.
//
// The engine's PDC (docs/AUDIO_ENGINE.md §7, proven in MixerTests against
// synthetic latent nodes) aligns paths by what nodes CLAIM. This file proves
// the claim of a hosted plugin travels: CLAP_EXT_LATENCY -> ClapInstance ->
// PluginNode::latencyFrames -> the same alignment machinery, with no
// plugin-specific code in the engine.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/MixerStripNode.h"
#include "engine/graph/RenderGraph.h"
#include "plugins/PluginInstanceManager.h"
#include "plugins/PluginNode.h"
#include "plugins/PluginRegistry.h"
#include "plugins/clap/ClapLibrary.h"
#include "project/Model.h"
#include "project/ProjectGraphCompiler.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
namespace fs = std::filesystem;

namespace {

constexpr FrameCount blockSize     = 256;
constexpr FrameCount pluginLatency = 64;   // what TestLatencyPlugin is built as

const std::string latencyUid = "com.incdaw.testlatency";

/// Emits a single 1.0 in the first block and silence forever after.
class ImpulseNode final : public Node {
public:
    explicit ImpulseNode(FrameCount at) noexcept : at_(at) {}

    void process(const ProcessContext& context) noexcept override
    {
        if (context.playPosition != 0)
            return;   // the graph clears buffers; silence is free

        if (at_ < context.frameCount)
            for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel)
                context.output.channel(channel)[at_] = Sample{1};
    }

    [[nodiscard]] const char* name() const noexcept override { return "Impulse"; }

private:
    FrameCount at_;
};

std::vector<Sample> render(CompiledGraph& graph, int blocks)
{
    AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    std::vector<Sample> samples;
    samples.reserve(static_cast<std::size_t>(blockSize) * static_cast<std::size_t>(blocks));

    for (int block = 0; block < blocks; ++block) {
        pool.buffer(0).clear();
        graph.process(pool.buffer(0), blockSize,
                      static_cast<FramePosition>(block) * blockSize);

        const Sample* left = pool.buffer(0).channel(0);
        samples.insert(samples.end(), left, left + blockSize);
    }

    return samples;
}

struct ScratchDir {
    fs::path path;

    explicit ScratchDir(const char* name) : path(fs::temp_directory_path() / name)
    {
        std::error_code code;
        fs::remove_all(path, code);
        fs::create_directories(path, code);
    }

    ~ScratchDir()
    {
        std::error_code code;
        fs::remove_all(path, code);
    }
};

/// Impulse into two parallel paths — one through the hosted latent plugin,
/// one direct — summed at the master. Alignment is the whole question.
/// The instance is BORROWED by the node (D-031): the caller keeps it alive
/// for as long as the returned graph renders.
std::unique_ptr<CompiledGraph> buildParallelGraph(plugins::ClapInstance* instance,
                                                  bool compensate)
{
    GraphBuilder builder;
    builder.setDelayCompensationEnabled(compensate);

    const auto source      = builder.addNode(std::make_unique<ImpulseNode>(0));
    const auto latent      = builder.addNode(std::make_unique<plugins::PluginNode>(instance));
    const auto latentStrip = builder.addNode(std::make_unique<dsp::MixerStripNode>());
    const auto direct      = builder.addNode(std::make_unique<dsp::MixerStripNode>());
    const auto summing     = builder.addNode(std::make_unique<dsp::MixerStripNode>());

    builder.connect(source, latent);
    builder.connect(latent, latentStrip);
    builder.connect(latentStrip, summing);
    builder.connect(source, direct);
    builder.connect(direct, summing);

    builder.setMaster(summing);

    return builder.compile(48000.0, blockSize, 2);
}

} // namespace

TEST_CASE("a created instance reports its CLAP latency, and none means zero")
{
    plugins::ClapLibrary library;
    std::string          error;

    REQUIRE(library.open(INCDAW_TESTLATENCY_PLUGIN, error));

    auto latent = library.create(latencyUid, 48000.0,
                                 static_cast<std::uint32_t>(blockSize), error);
    REQUIRE(latent != nullptr);
    CHECK(latent->latencyFrames() == pluginLatency);

    plugins::ClapLibrary gainLibrary;
    REQUIRE(gainLibrary.open(INCDAW_TESTGAIN_PLUGIN, error));

    auto gain = gainLibrary.create("com.incdaw.testgain", 48000.0,
                                   static_cast<std::uint32_t>(blockSize), error);
    REQUIRE(gain != nullptr);
    CHECK(gain->latencyFrames() == 0);
}

TEST_CASE("EXIT CRITERION: a hosted plugin's reported latency aligns parallel paths")
{
    plugins::ClapLibrary library;
    std::string          error;
    REQUIRE(library.open(INCDAW_TESTLATENCY_PLUGIN, error));

    // One instance per graph, kept alive here for as long as its graph is.
    auto first  = library.create(latencyUid, 48000.0,
                                 static_cast<std::uint32_t>(blockSize), error);
    auto second = library.create(latencyUid, 48000.0,
                                 static_cast<std::uint32_t>(blockSize), error);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    // Without compensation the two arrivals are 64 frames apart — the comb a
    // mixer without PDC produces. This half proves the test can see the flaw.
    // Amplitudes are measured relative to one arrival, because each strip
    // applies the centre pan law on the way through.
    auto uncompensated = buildParallelGraph(first.get(), false);
    REQUIRE(uncompensated != nullptr);

    const auto   combed  = render(*uncompensated, 2);
    const Sample arrival = combed[0];

    REQUIRE(arrival > Sample{0});
    CHECK(combed[pluginLatency] == arrival);

    // With compensation both arrive together, once, at the plugin's latency.
    auto compensated = buildParallelGraph(second.get(), true);
    REQUIRE(compensated != nullptr);
    CHECK(compensated->latencyFrames() == pluginLatency);

    const auto aligned = render(*compensated, 2);

    for (std::size_t index = 0; index < aligned.size(); ++index) {
        if (index == static_cast<std::size_t>(pluginLatency))
            REQUIRE(aligned[index] == arrival * Sample{2});
        else
            REQUIRE(aligned[index] == Sample{0});
    }
}

TEST_CASE("a project whose insert is latent reports the graph's total latency")
{
    ScratchDir folder{"incdaw-latency-plugins"};
    fs::copy_file(INCDAW_TESTLATENCY_PLUGIN, folder.path / "latency.clap");

    plugins::PluginRegistry registry;
    REQUIRE(registry.scanDirectory(folder.path, INCDAW_PLUGINSCAN_BINARY) == 1);

    plugins::PluginInstanceManager manager{registry};

    project::Project project;
    const project::EntityId channel = project.addChannel("Channel 1").id;
    const project::EntityId pattern = project.addPattern("Pattern 1").id;

    project::MidiEvent note;
    note.type     = project::MidiEventType::note;
    note.tick     = 0;
    note.duration = engine::ticksPerQuarterNote;
    note.key      = 60;
    note.value    = 100;
    project.findPattern(pattern)->contentFor(channel).events.push_back(note);

    for (project::MixerNode& node : project.mixerNodes()) {
        if (node.id != project.masterMixerNode())
            continue;

        project::PluginSlot slot;
        slot.id            = project.ids().next();
        slot.plugin.format = plugins::Format::clap;
        slot.plugin.uid    = latencyUid;
        node.inserts.push_back(slot);
    }

    engine::TempoMap tempo;
    tempo.setSampleRate(48000.0);

    project::GraphCompileOptions options;
    options.sampleRate    = 48000.0;
    options.maxBlockSize  = blockSize;
    options.pattern       = pattern;
    options.insertFactory = [&manager](const project::PluginSlot& slot, std::string& error) {
        return manager.createInsert(slot.id.value(), slot.plugin, 48000.0,
                                    static_cast<std::uint32_t>(blockSize), error);
    };

    auto compiled = project::compileProjectGraph(project, tempo, options);
    REQUIRE(compiled);
    CHECK(compiled.warnings.empty());
    CHECK(compiled.graph->latencyFrames() == pluginLatency);
}
