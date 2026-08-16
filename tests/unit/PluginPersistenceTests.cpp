// Phase 13 — instances outlive graphs (D-031).
//
// Graphs are rebuilt on every edit. If an instance died with its node, a
// plugin would reset — audibly, and under any open editor — every time the
// user added a note. The manager therefore keys instances by SLOT, hands
// the node a borrowed pointer, and disposes an instance only when its slot
// leaves the project.

#include "doctest.h"

#include "plugins/PluginInstanceManager.h"
#include "plugins/PluginRegistry.h"

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

using namespace incdaw;
namespace fs = std::filesystem;

namespace {

constexpr std::uint32_t blockSize = 256;

const std::string gainUid    = "com.incdaw.testgain";
const std::string latencyUid = "com.incdaw.testlatency";

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

std::vector<std::uint8_t> gainBlob(double gain)
{
    std::vector<std::uint8_t> blob(sizeof(gain));
    std::memcpy(blob.data(), &gain, sizeof(gain));
    return blob;
}

float processOnes(plugins::HostedPlugin& instance)
{
    std::vector<float> left(blockSize, 1.0f);
    std::vector<float> right(blockSize, 1.0f);
    REQUIRE(instance.process(left.data(), right.data(), blockSize));
    return left.front();
}

struct Harness {
    ScratchDir              folder{"incdaw-persistence-plugins"};
    plugins::PluginRegistry registry;

    Harness()
    {
        fs::copy_file(INCDAW_TESTGAIN_PLUGIN, folder.path / "gain.clap");
        fs::copy_file(INCDAW_TESTLATENCY_PLUGIN, folder.path / "latency.clap");
        REQUIRE(registry.scanDirectory(folder.path, INCDAW_PLUGINSCAN_BINARY) == 2);
    }

    plugins::PluginIdentifier plugin(const std::string& uid)
    {
        plugins::PluginIdentifier identifier;
        identifier.format = plugins::Format::clap;
        identifier.uid    = uid;
        return identifier;
    }
};

} // namespace

TEST_CASE("the same slot gets the same instance across rebuilds, live state intact")
{
    Harness harness;
    plugins::PluginInstanceManager manager{harness.registry};

    std::string error;
    auto first = manager.createInsert(7, harness.plugin(gainUid), 48000.0, blockSize, error);
    REQUIRE(first != nullptr);

    plugins::HostedPlugin* instance = manager.instanceFor(7);
    REQUIRE(instance != nullptr);

    // "The user tweaked the plugin": its gain becomes 2.0, live.
    const auto blob = gainBlob(2.0);
    REQUIRE(instance->loadState(blob.data(), blob.size()));

    // "The user added a note": the graph rebuilds, the factory runs again.
    auto second = manager.createInsert(7, harness.plugin(gainUid), 48000.0, blockSize, error);
    REQUIRE(second != nullptr);

    CHECK(manager.instanceFor(7) == instance);
    CHECK(manager.liveInstanceCount() == 1);
    CHECK(processOnes(*instance) == 2.0f);   // the tweak survived the rebuild
}

TEST_CASE("instances are disposed only when their slot leaves the project")
{
    Harness harness;
    plugins::PluginInstanceManager manager{harness.registry};

    std::string error;
    REQUIRE(manager.createInsert(1, harness.plugin(gainUid), 48000.0, blockSize, error));
    REQUIRE(manager.createInsert(2, harness.plugin(gainUid), 48000.0, blockSize, error));
    CHECK(manager.liveInstanceCount() == 2);

    manager.retainOnlyInstances({1});

    CHECK(manager.liveInstanceCount() == 1);
    CHECK(manager.instanceFor(1) != nullptr);
    CHECK(manager.instanceFor(2) == nullptr);
}

TEST_CASE("changed activation terms recreate the instance but carry its state")
{
    Harness harness;
    plugins::PluginInstanceManager manager{harness.registry};

    std::string error;
    REQUIRE(manager.createInsert(9, harness.plugin(gainUid), 48000.0, blockSize, error));

    plugins::HostedPlugin* before = manager.instanceFor(9);
    REQUIRE(before != nullptr);

    const auto blob = gainBlob(2.0);
    REQUIRE(before->loadState(blob.data(), blob.size()));

    // The device changed its sample rate: a new activation is unavoidable,
    // but the plugin must not audibly reset.
    REQUIRE(manager.createInsert(9, harness.plugin(gainUid), 44100.0, blockSize, error));

    plugins::HostedPlugin* after = manager.instanceFor(9);
    REQUIRE(after != nullptr);
    CHECK(manager.liveInstanceCount() == 1);

    // Not a pointer comparison — the allocator may reuse the address. The
    // recreation shows in behaviour: the state survived the new activation.
    CHECK(processOnes(*after) == 2.0f);
}

TEST_CASE("a slot whose plugin changed starts fresh")
{
    Harness harness;
    plugins::PluginInstanceManager manager{harness.registry};

    std::string error;
    REQUIRE(manager.createInsert(5, harness.plugin(gainUid), 48000.0, blockSize, error));

    plugins::HostedPlugin* gain = manager.instanceFor(5);
    REQUIRE(gain != nullptr);

    const auto blob = gainBlob(2.0);
    REQUIRE(gain->loadState(blob.data(), blob.size()));

    // The user replaced the plugin in the slot: no state carries across —
    // one plugin's blob means nothing to another.
    REQUIRE(manager.createInsert(5, harness.plugin(latencyUid), 48000.0, blockSize, error));

    plugins::HostedPlugin* latent = manager.instanceFor(5);
    REQUIRE(latent != nullptr);
    CHECK(manager.liveInstanceCount() == 1);

    // The slot now hosts the OTHER plugin — its latency proves it — and the
    // gain plugin's blob did not leak into it: a fresh latency plugin delays
    // ones into leading silence rather than scaling them.
    CHECK(latent->latencyFrames() == 64);
    CHECK(processOnes(*latent) == 0.0f);
}
