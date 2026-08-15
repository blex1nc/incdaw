// Phase 13 — plugin parameters: discovery, the generalised registry, and the
// event queue.
//
// Three layers, tested separately before they are tested together. The
// registry's new sink appliers need no plugin binary at all; discovery and
// event delivery talk to the test-suite's own CLAP gain; the exit criterion
// compiles a project whose automation lane drives a hosted plugin's parameter
// through the same generic subsystem that drives a fader.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/graph/ParameterSink.h"
#include "plugins/PluginInstanceManager.h"
#include "plugins/PluginRegistry.h"
#include "plugins/clap/ClapLibrary.h"
#include "project/Model.h"
#include "project/ParameterRegistry.h"
#include "project/ProjectGraphCompiler.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace incdaw;
namespace fs = std::filesystem;

namespace {

using engine::FrameCount;
using engine::FramePosition;
using engine::Sample;

constexpr FrameCount    blockSize   = 256;
constexpr int           blockCount  = 8;
constexpr std::uint32_t gainParamId = 0;

const std::string testGainUid = "com.incdaw.testgain";

/// Remembers every value it is handed, so a registry applier's output is
/// inspectable without an audio graph.
struct RecordingSink final : engine::ParameterSink {
    std::vector<std::pair<std::uint32_t, double>> received;

    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override
    {
        received.emplace_back(parameterId, plainValue);
    }
};

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

/// A project with one audible channel playing one note, so that compiling it
/// produces a signal for a hosted insert to act on.
struct ParameterFixture {
    project::Project  project;
    project::EntityId channel;
    project::EntityId pattern;
    engine::TempoMap  tempo;

    ParameterFixture()
    {
        channel = project.addChannel("Channel 1").id;
        project.channels().front().volume = 4.0;

        pattern = project.addPattern("Pattern 1").id;

        project::MidiEvent note;
        note.type     = project::MidiEventType::note;
        note.tick     = 0;
        note.duration = engine::ticksPerQuarterNote * 4;
        note.key      = 60;
        note.value    = 100;
        project.findPattern(pattern)->contentFor(channel).events.push_back(note);

        tempo.setSampleRate(48000.0);
    }

    project::MixerNode& master()
    {
        for (project::MixerNode& node : project.mixerNodes())
            if (node.id == project.masterMixerNode())
                return node;

        FAIL("the project has no master mixer node");
        return project.mixerNodes().front();
    }

    project::PluginSlot& addMasterInsert(std::string uid)
    {
        project::PluginSlot slot;
        slot.id            = project.ids().next();
        slot.plugin.format = plugins::Format::clap;
        slot.plugin.uid    = std::move(uid);

        master().inserts.push_back(slot);
        return master().inserts.back();
    }

    [[nodiscard]] project::CompiledProjectGraph compile(
        project::InsertFactory factory = {},
        const project::ParameterRegistry* parameters = nullptr)
    {
        project::GraphCompileOptions options;
        options.sampleRate    = 48000.0;
        options.maxBlockSize  = blockSize;
        options.pattern       = pattern;
        options.masterGain    = Sample{1};
        options.insertFactory = std::move(factory);
        options.parameters    = parameters;

        return project::compileProjectGraph(project, tempo, options);
    }
};

/// Renders the graph and returns the left channel, blocks concatenated.
std::vector<Sample> render(engine::CompiledGraph& graph)
{
    engine::AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    std::vector<Sample> samples;
    samples.reserve(static_cast<std::size_t>(blockSize) * blockCount);

    for (int block = 0; block < blockCount; ++block) {
        pool.buffer(0).clear();
        graph.process(pool.buffer(0), blockSize,
                      static_cast<FramePosition>(block) * blockSize);

        const Sample* left = pool.buffer(0).channel(0);
        samples.insert(samples.end(), left, left + blockSize);
    }

    return samples;
}

/// Renders with the realtime guard armed around the audio work only.
std::vector<Sample> renderUnderRealtimeGuard(engine::CompiledGraph& graph)
{
    engine::AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    std::vector<Sample> samples(static_cast<std::size_t>(blockSize) * blockCount, Sample{0});

    for (int block = 0; block < blockCount; ++block) {
        pool.buffer(0).clear();

        {
            const engine::rt::ScopedRealtimeContext realtimeScope;
            graph.process(pool.buffer(0), blockSize,
                          static_cast<FramePosition>(block) * blockSize);
        }

        const Sample* left = pool.buffer(0).channel(0);
        std::copy(left, left + blockSize,
                  samples.begin() + static_cast<std::ptrdiff_t>(block) * blockSize);
    }

    return samples;
}

[[nodiscard]] bool anyNonZero(const std::vector<Sample>& samples)
{
    return std::any_of(samples.begin(), samples.end(),
                       [](Sample value) { return value != Sample{0}; });
}

} // namespace

// ── The registry, generalised ────────────────────────────────────────────────

TEST_CASE("a plugin parameter registers like any other, keyed per plugin type")
{
    project::ParameterRegistry registry = project::ParameterRegistry::withBuiltins();
    const std::size_t builtins = registry.size();

    plugins::PluginParameterInfo info;
    info.id           = 7;
    info.name         = "Cutoff";
    info.minValue     = 0.0;
    info.maxValue     = 2.0;
    info.defaultValue = 1.0;

    registry.registerPluginParameters("com.acme.synth", {info});
    CHECK(registry.size() == builtins + 1);

    const auto* entry = registry.find(
        project::ParameterRegistry::pluginParameterKey("com.acme.synth", 7));
    REQUIRE(entry != nullptr);

    // The entry holds a SINK applier: its target is a hosted instance, not a
    // strip, and the compiler resolves the lane's target accordingly.
    const auto* apply = std::get_if<project::ParameterRegistry::SinkApplier>(&entry->apply);
    REQUIRE(apply != nullptr);

    RecordingSink sink;
    (*apply)(sink, 0.0f);
    (*apply)(sink, 0.5f);
    (*apply)(sink, 1.0f);

    REQUIRE(sink.received.size() == 3);
    CHECK(sink.received[0] == std::pair<std::uint32_t, double>{7, 0.0});
    CHECK(sink.received[1] == std::pair<std::uint32_t, double>{7, 1.0});
    CHECK(sink.received[2] == std::pair<std::uint32_t, double>{7, 2.0});

    // The built-ins are untouched, and still strip appliers.
    const auto* volume = registry.find("volume");
    REQUIRE(volume != nullptr);
    CHECK(std::get_if<project::ParameterRegistry::StripApplier>(&volume->apply) != nullptr);

    // Re-registration replaces, so rediscovery on every graph rebuild is safe.
    registry.registerPluginParameters("com.acme.synth", {info});
    CHECK(registry.size() == builtins + 1);
}

TEST_CASE("a stepped parameter lands on whole steps")
{
    project::ParameterRegistry registry;

    plugins::PluginParameterInfo info;
    info.id       = 3;
    info.minValue = 0.0;
    info.maxValue = 4.0;
    info.stepped  = true;

    registry.registerPluginParameters("com.acme.selector", {info});

    const auto* entry = registry.find(
        project::ParameterRegistry::pluginParameterKey("com.acme.selector", 3));
    REQUIRE(entry != nullptr);

    const auto* apply = std::get_if<project::ParameterRegistry::SinkApplier>(&entry->apply);
    REQUIRE(apply != nullptr);

    RecordingSink sink;
    (*apply)(sink, 0.6f);   // 2.4 un-rounded

    REQUIRE(sink.received.size() == 1);
    CHECK(sink.received[0].second == 2.0);
}

// ── Discovery ────────────────────────────────────────────────────────────────

TEST_CASE("a created instance reports the plugin's parameters in plain terms")
{
    plugins::ClapLibrary library;
    std::string          error;

    REQUIRE(library.open(INCDAW_TESTGAIN_PLUGIN, error));

    auto instance = library.create(testGainUid, 48000.0, blockSize, error);
    REQUIRE(instance != nullptr);

    REQUIRE(instance->parameters().size() == 1);

    const plugins::PluginParameterInfo& gain = instance->parameters().front();
    CHECK(gain.id == gainParamId);
    CHECK(gain.name == "Gain");
    CHECK(gain.minValue == 0.0);
    CHECK(gain.maxValue == 2.0);
    CHECK(gain.defaultValue == 0.5);
    CHECK(!gain.stepped);
}

TEST_CASE("the instance manager caches discovery per plugin type")
{
    ScratchDir folder{"incdaw-param-discovery"};
    fs::copy_file(INCDAW_TESTGAIN_PLUGIN, folder.path / "gain.clap");

    plugins::PluginRegistry registry;
    REQUIRE(registry.scanDirectory(folder.path, INCDAW_PLUGINSCAN_BINARY) == 1);

    plugins::PluginInstanceManager manager{registry};

    CHECK(manager.parametersFor(testGainUid) == nullptr);   // nothing created yet

    plugins::PluginIdentifier identifier;
    identifier.format = plugins::Format::clap;
    identifier.uid    = testGainUid;

    std::string error;
    auto        first = manager.createInsert(identifier, 48000.0, blockSize, error);
    REQUIRE(first != nullptr);

    const auto* discovered = manager.parametersFor(testGainUid);
    REQUIRE(discovered != nullptr);
    REQUIRE(discovered->size() == 1);
    CHECK(discovered->front().id == gainParamId);

    // A second instance reuses the cached list rather than rediscovering.
    auto second = manager.createInsert(identifier, 48000.0, blockSize, error);
    REQUIRE(second != nullptr);
    CHECK(manager.parametersFor(testGainUid) == discovered);

    CHECK(manager.parametersFor("com.nobody.nothing") == nullptr);
}

// ── The event queue ──────────────────────────────────────────────────────────

TEST_CASE("queued values reach the plugin as events in its next process call")
{
    plugins::ClapLibrary library;
    std::string          error;

    REQUIRE(library.open(INCDAW_TESTGAIN_PLUGIN, error));

    auto instance = library.create(testGainUid, 48000.0, blockSize, error);
    REQUIRE(instance != nullptr);

    std::vector<float> left(blockSize, 1.0f);
    std::vector<float> right(blockSize, 1.0f);

    const auto processOnes = [&] {
        std::fill(left.begin(), left.end(), 1.0f);
        std::fill(right.begin(), right.end(), 1.0f);
        REQUIRE(instance->process(left.data(), right.data(),
                                  static_cast<std::uint32_t>(blockSize)));
    };

    // No events queued: the plugin's default, -6 dB.
    processOnes();
    CHECK(left.front() == 0.5f);
    CHECK(left.back() == 0.5f);

    instance->setParameter(gainParamId, 2.0);
    processOnes();
    CHECK(left.front() == 2.0f);

    instance->setParameter(gainParamId, 1.0);
    processOnes();
    CHECK(left.front() == 1.0f);

    // Two values queued in one block arrive in order: the last one wins, which
    // is what "the value at block start" means to per-block automation.
    instance->setParameter(gainParamId, 0.25);
    instance->setParameter(gainParamId, 0.75);
    processOnes();
    CHECK(left.front() == 0.75f);

    // The value holds once set: no events, no change.
    processOnes();
    CHECK(left.front() == 0.75f);
}

// ── End to end ───────────────────────────────────────────────────────────────

TEST_CASE("EXIT CRITERION: a hosted plugin parameter automates through the generic subsystem")
{
    ScratchDir folder{"incdaw-param-exit"};
    fs::copy_file(INCDAW_TESTGAIN_PLUGIN, folder.path / "gain.clap");

    plugins::PluginRegistry pluginRegistry;
    REQUIRE(pluginRegistry.scanDirectory(folder.path, INCDAW_PLUGINSCAN_BINARY) == 1);

    plugins::PluginInstanceManager manager{pluginRegistry};

    ParameterFixture fixture;

    auto baseline = fixture.compile();
    REQUIRE(baseline);
    const auto dry = render(*baseline.graph);
    REQUIRE(anyNonZero(dry));

    const project::PluginSlot& slot = fixture.addMasterInsert(testGainUid);

    // Wired exactly the way the application wires it: the factory creates the
    // instance AND lands its discovered parameters in the registry, before
    // automation lanes bind later in the same compile.
    project::ParameterRegistry parameters = project::ParameterRegistry::withBuiltins();

    const auto factory = [&](const project::PluginSlot& insert, std::string& error)
        -> std::unique_ptr<engine::Node> {
        auto node = manager.createInsert(insert.plugin, 48000.0, blockSize, error);

        if (node != nullptr)
            if (const auto* discovered = manager.parametersFor(insert.plugin.uid))
                parameters.registerPluginParameters(insert.plugin.uid, *discovered);

        return node;
    };

    // The lane: nothing in it names the plugin's parameter beyond the key, and
    // nothing anywhere else does either — that is the exit criterion.
    project::AutomationLane& lane = fixture.project.addAutomationLane(
        slot.id, project::ParameterRegistry::pluginParameterKey(testGainUid, gainParamId));

    project::AutomationPoint point;
    point.tick  = 0;
    point.value = 0.5;   // normalised; plain range 0..2, so the gain becomes 1.0
    point.curve = project::AutomationCurve::hold;
    lane.points.push_back(point);

    auto compiled = fixture.compile(factory, &parameters);
    REQUIRE(compiled);
    CHECK(compiled.warnings.empty());
    REQUIRE(compiled.automation != nullptr);
    CHECK(compiled.automation->bindingCount() == 1);

    engine::rt::resetViolations();

    const auto wet = renderUnderRealtimeGuard(*compiled.graph);
    REQUIRE(wet.size() == dry.size());

    // Automation drives the -6 dB plugin to unity gain, so the insert becomes
    // transparent. The first block is exempt: whether the automation node runs
    // before or after the plugin within one block is a graph-order detail, and
    // per-block delivery means the value is guaranteed from the next block on.
    for (std::size_t index = blockSize; index < wet.size(); ++index)
        REQUIRE(wet[index] == dry[index]);

    // Queueing a parameter value is audio-thread work now: it must not allocate.
    if (engine::rt::guardEnabled()) {
        CHECK(engine::rt::allocationViolations() == 0);
        CHECK(engine::rt::deallocationViolations() == 0);
    }
}

TEST_CASE("a mismatched key/target pair is data, not an error")
{
    ScratchDir folder{"incdaw-param-mismatch"};
    fs::copy_file(INCDAW_TESTGAIN_PLUGIN, folder.path / "gain.clap");

    plugins::PluginRegistry pluginRegistry;
    REQUIRE(pluginRegistry.scanDirectory(folder.path, INCDAW_PLUGINSCAN_BINARY) == 1);

    plugins::PluginInstanceManager manager{pluginRegistry};

    ParameterFixture fixture;
    const project::PluginSlot& slot = fixture.addMasterInsert(testGainUid);

    project::ParameterRegistry parameters = project::ParameterRegistry::withBuiltins();

    const auto factory = [&](const project::PluginSlot& insert, std::string& error)
        -> std::unique_ptr<engine::Node> {
        auto node = manager.createInsert(insert.plugin, 48000.0, blockSize, error);

        if (node != nullptr)
            if (const auto* discovered = manager.parametersFor(insert.plugin.uid))
                parameters.registerPluginParameters(insert.plugin.uid, *discovered);

        return node;
    };

    // A strip parameter aimed at an insert slot, and a plugin parameter aimed
    // at a strip: both are stale data a project can legitimately contain, and
    // both must be skipped without failing the compile.
    project::AutomationLane& stripKeyOnSlot =
        fixture.project.addAutomationLane(slot.id, "volume");

    project::AutomationLane& pluginKeyOnStrip = fixture.project.addAutomationLane(
        fixture.project.masterMixerNode(),
        project::ParameterRegistry::pluginParameterKey(testGainUid, gainParamId));

    project::AutomationPoint point;
    point.tick  = 0;
    point.value = 1.0;
    point.curve = project::AutomationCurve::hold;
    stripKeyOnSlot.points.push_back(point);
    pluginKeyOnStrip.points.push_back(point);

    auto compiled = fixture.compile(factory, &parameters);
    REQUIRE(compiled);
    CHECK(compiled.warnings.empty());

    // Neither lane bound — and nothing crashed, silenced the mix, or refused
    // to compile. The insert still processes at its default gain.
    CHECK(compiled.automation == nullptr);

    const auto rendered = render(*compiled.graph);
    CHECK(anyNonZero(rendered));
}
