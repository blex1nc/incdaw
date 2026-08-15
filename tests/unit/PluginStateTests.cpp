// Phase 13 — plugin state: capture, the package, and restore.
//
// The state chain has three links — the instance's CLAP_EXT_STATE bridge,
// the compiled graph's slot-to-carrier map, and the package's blob files —
// tested first separately, then as one round trip: a project whose plugin
// remembers what the user made it, across save, load and recompile.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/graph/StateIO.h"
#include "plugins/PluginInstanceManager.h"
#include "plugins/PluginRegistry.h"
#include "plugins/clap/ClapLibrary.h"
#include "project/Model.h"
#include "project/PluginStateFiles.h"
#include "project/ProjectFile.h"
#include "project/ProjectGraphCompiler.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
namespace fs = std::filesystem;

namespace {

using engine::FrameCount;
using engine::FramePosition;
using engine::Sample;

constexpr FrameCount blockSize  = 256;
constexpr int        blockCount = 8;

const std::string testGainUid = "com.incdaw.testgain";

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

/// The test plugin's state is its gain, serialised as the double's 8 bytes.
std::vector<std::uint8_t> gainBlob(double gain)
{
    std::vector<std::uint8_t> blob(sizeof(gain));
    std::memcpy(blob.data(), &gain, sizeof(gain));
    return blob;
}

/// A project with one audible channel playing one note, and the plumbing to
/// compile it against the real plugin host.
struct StateFixture {
    project::Project  project;
    project::EntityId channel;
    project::EntityId pattern;
    engine::TempoMap  tempo;

    StateFixture()
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

    [[nodiscard]] project::CompiledProjectGraph compile(project::InsertFactory factory = {})
    {
        project::GraphCompileOptions options;
        options.sampleRate    = 48000.0;
        options.maxBlockSize  = blockSize;
        options.pattern       = pattern;
        options.masterGain    = Sample{1};
        options.insertFactory = std::move(factory);

        return project::compileProjectGraph(project, tempo, options);
    }
};

/// Compiles a LOADED project (whose first pattern id is unknown to the
/// caller) against the same options the fixture uses.
project::CompiledProjectGraph compileLoaded(const project::Project& loaded,
                                            const engine::TempoMap& tempo,
                                            project::InsertFactory  factory)
{
    project::GraphCompileOptions options;
    options.sampleRate    = 48000.0;
    options.maxBlockSize  = blockSize;
    options.masterGain    = Sample{1};
    options.insertFactory = std::move(factory);

    return project::compileProjectGraph(loaded, tempo, options);
}

project::InsertFactory factoryFor(plugins::PluginInstanceManager& manager)
{
    return [&manager](const project::PluginSlot& slot,
                      std::string& error) -> std::unique_ptr<engine::Node> {
        return manager.createInsert(slot.plugin, 48000.0, blockSize, error);
    };
}

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

[[nodiscard]] bool anyNonZero(const std::vector<Sample>& samples)
{
    return std::any_of(samples.begin(), samples.end(),
                       [](Sample value) { return value != Sample{0}; });
}

} // namespace

// ── The instance's state bridge ──────────────────────────────────────────────

TEST_CASE("an instance's state round-trips through save and load")
{
    plugins::ClapLibrary library;
    std::string          error;

    REQUIRE(library.open(INCDAW_TESTGAIN_PLUGIN, error));

    auto first = library.create(testGainUid, 48000.0, blockSize, error);
    REQUIRE(first != nullptr);

    // Make the state worth keeping: gain 2.0 instead of the default 0.5.
    first->setParameter(0, 2.0);

    std::vector<float> left(blockSize, 1.0f);
    std::vector<float> right(blockSize, 1.0f);
    REQUIRE(first->process(left.data(), right.data(), static_cast<std::uint32_t>(blockSize)));
    REQUIRE(left.front() == 2.0f);

    std::vector<std::uint8_t> blob;
    REQUIRE(first->saveState(blob));
    CHECK(blob.size() == sizeof(double));

    // A fresh instance starts at the default and adopts the blob.
    auto second = library.create(testGainUid, 48000.0, blockSize, error);
    REQUIRE(second != nullptr);
    REQUIRE(second->loadState(blob.data(), blob.size()));

    std::fill(left.begin(), left.end(), 1.0f);
    std::fill(right.begin(), right.end(), 1.0f);
    REQUIRE(second->process(left.data(), right.data(), static_cast<std::uint32_t>(blockSize)));
    CHECK(left.front() == 2.0f);

    // A blob that is not the plugin's is refused, and the state stands.
    const std::vector<std::uint8_t> garbage{0x01, 0x02, 0x03};
    CHECK(!second->loadState(garbage.data(), garbage.size()));

    std::fill(left.begin(), left.end(), 1.0f);
    std::fill(right.begin(), right.end(), 1.0f);
    REQUIRE(second->process(left.data(), right.data(), static_cast<std::uint32_t>(blockSize)));
    CHECK(left.front() == 2.0f);
}

// ── The whole chain ──────────────────────────────────────────────────────────

TEST_CASE("EXIT CRITERION: plugin state survives save, load and recompile")
{
    ScratchDir plugins{"incdaw-state-plugins"};
    ScratchDir packages{"incdaw-state-package"};
    fs::copy_file(INCDAW_TESTGAIN_PLUGIN, plugins.path / "gain.clap");

    plugins::PluginRegistry registry;
    REQUIRE(registry.scanDirectory(plugins.path, INCDAW_PLUGINSCAN_BINARY) == 1);

    plugins::PluginInstanceManager manager{registry};

    StateFixture fixture;

    auto baseline = fixture.compile();
    REQUIRE(baseline);
    const auto dry = render(*baseline.graph);
    REQUIRE(anyNonZero(dry));

    const project::EntityId slotId = fixture.addMasterInsert(testGainUid).id;

    auto compiled = fixture.compile(factoryFor(manager));
    REQUIRE(compiled);
    CHECK(compiled.warnings.empty());

    // The compiled graph exposes the slot's state carrier; the user "edits"
    // the plugin by handing it a state whose gain is 2.0.
    engine::StateIO* live = compiled.insertStateFor(slotId);
    REQUIRE(live != nullptr);

    const auto blob = gainBlob(2.0);
    REQUIRE(live->loadState(blob.data(), blob.size()));

    const auto edited = render(*compiled.graph);
    REQUIRE(edited.size() == dry.size());
    for (std::size_t index = 0; index < edited.size(); ++index)
        REQUIRE(edited[index] == dry[index] * Sample{2});

    // Save: capture FIRST, so the recorded stateFile lands in project.json.
    const fs::path package = packages.path / "State.incdaw";

    const auto captureWarnings =
        project::capturePluginState(fixture.project, compiled, package);
    CHECK(captureWarnings.empty());

    const project::PluginSlot& savedSlot = fixture.master().inserts.front();
    CHECK(savedSlot.stateFile == "plugins/insert-" + std::to_string(slotId.value()) + ".state");
    CHECK(fs::exists(package / savedSlot.stateFile));

    REQUIRE(project::ProjectFile::save(fixture.project, package));

    // Load into a fresh project: the slot remembers its blob's path...
    project::Project loaded;
    REQUIRE(project::ProjectFile::load(loaded, package));

    const project::MixerNode* master = nullptr;
    for (const project::MixerNode& node : loaded.mixerNodes())
        if (node.id == loaded.masterMixerNode())
            master = &node;

    REQUIRE(master != nullptr);
    REQUIRE(master->inserts.size() == 1);
    CHECK(master->inserts.front().stateFile == savedSlot.stateFile);

    // ...and after recompile + restore, the plugin sounds like it did.
    auto recompiled = compileLoaded(loaded, fixture.tempo, factoryFor(manager));
    REQUIRE(recompiled);

    const auto restoreWarnings = project::restorePluginState(loaded, recompiled, package);
    CHECK(restoreWarnings.empty());

    const auto restored = render(*recompiled.graph);
    REQUIRE(restored.size() == dry.size());
    for (std::size_t index = 0; index < restored.size(); ++index)
        REQUIRE(restored[index] == dry[index] * Sample{2});
}

TEST_CASE("a missing plugin keeps its recorded state for when it returns")
{
    ScratchDir packages{"incdaw-state-missing"};

    StateFixture fixture;
    project::PluginSlot& slot = fixture.addMasterInsert("com.acme.gone");
    slot.stateFile = "plugins/insert-" + std::to_string(slot.id.value()) + ".state";

    const fs::path package = packages.path / "Missing.incdaw";
    std::error_code code;
    fs::create_directories(package / "plugins", code);

    {
        std::ofstream blobFile(package / slot.stateFile, std::ios::binary);
        const auto    blob = gainBlob(1.5);
        blobFile.write(reinterpret_cast<const char*>(blob.data()),
                       static_cast<std::streamsize>(blob.size()));
    }

    // No factory: the plugin cannot be built, so the slot has no live state
    // carrier — a placeholder, exactly the §6 missing-plugin case.
    auto compiled = fixture.compile();
    REQUIRE(compiled);
    CHECK(compiled.insertStateFor(slot.id) == nullptr);

    // Capturing must not touch what it cannot see...
    const auto captureWarnings =
        project::capturePluginState(fixture.project, compiled, package);
    CHECK(captureWarnings.empty());
    CHECK(fixture.master().inserts.front().stateFile == slot.stateFile);
    CHECK(fs::exists(package / slot.stateFile));

    // ...and restoring skips it silently. The blob waits on disk.
    const auto restoreWarnings =
        project::restorePluginState(fixture.project, compiled, package);
    CHECK(restoreWarnings.empty());
}

TEST_CASE("a blob the plugin rejects is a warning, and the plugin plays its defaults")
{
    ScratchDir plugins{"incdaw-state-reject-plugins"};
    ScratchDir packages{"incdaw-state-reject"};
    fs::copy_file(INCDAW_TESTGAIN_PLUGIN, plugins.path / "gain.clap");

    plugins::PluginRegistry registry;
    REQUIRE(registry.scanDirectory(plugins.path, INCDAW_PLUGINSCAN_BINARY) == 1);

    plugins::PluginInstanceManager manager{registry};

    StateFixture fixture;

    auto baseline = fixture.compile();
    REQUIRE(baseline);
    const auto dry = render(*baseline.graph);
    REQUIRE(anyNonZero(dry));

    project::PluginSlot& slot = fixture.addMasterInsert(testGainUid);
    slot.stateFile = "plugins/insert-" + std::to_string(slot.id.value()) + ".state";

    const fs::path package = packages.path / "Reject.incdaw";
    std::error_code code;
    fs::create_directories(package / "plugins", code);

    {
        std::ofstream blobFile(package / slot.stateFile, std::ios::binary);
        blobFile << "not a state blob";
    }

    auto compiled = fixture.compile(factoryFor(manager));
    REQUIRE(compiled);

    const auto warnings = project::restorePluginState(fixture.project, compiled, package);
    REQUIRE(warnings.size() == 1);
    CHECK(warnings.front().find("rejected") != std::string::npos);

    // The plugin still runs — at its default -6 dB.
    const auto rendered = render(*compiled.graph);
    REQUIRE(rendered.size() == dry.size());
    for (std::size_t index = 0; index < rendered.size(); ++index)
        REQUIRE(rendered[index] == dry[index] * Sample{0.5});
}

TEST_CASE("an unreadable state file is a warning, not a failed load")
{
    ScratchDir plugins{"incdaw-state-unreadable-plugins"};
    ScratchDir packages{"incdaw-state-unreadable"};
    fs::copy_file(INCDAW_TESTGAIN_PLUGIN, plugins.path / "gain.clap");

    plugins::PluginRegistry registry;
    REQUIRE(registry.scanDirectory(plugins.path, INCDAW_PLUGINSCAN_BINARY) == 1);

    plugins::PluginInstanceManager manager{registry};

    StateFixture fixture;
    project::PluginSlot& slot = fixture.addMasterInsert(testGainUid);
    slot.stateFile = "plugins/never-written.state";

    auto compiled = fixture.compile(factoryFor(manager));
    REQUIRE(compiled);

    const auto warnings = project::restorePluginState(fixture.project, compiled,
                                                      packages.path / "Nowhere.incdaw");
    REQUIRE(warnings.size() == 1);
    CHECK(warnings.front().find("not read") != std::string::npos);

    CHECK(anyNonZero(render(*compiled.graph)));
}
