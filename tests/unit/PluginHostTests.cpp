// Phase 13 (part 1) — CLAP hosting.
//
// Tested against the suite's OWN plugins: a well-behaved gain and a hostile
// one that segfaults on load. The load-bearing property is the isolation
// contract: the hostile plugin kills the SCANNER, and this process — the
// host — proves it survived by finishing the test.

#include "doctest.h"

#include "plugins/PluginScan.h"
#include "plugins/clap/ClapLibrary.h"

#include <cmath>
#include <vector>

using namespace incdaw;

TEST_CASE("a well-behaved plugin scans, loads and processes in-process")
{
    plugins::ClapLibrary library;
    std::string error;

    REQUIRE(library.open(INCDAW_TESTGAIN_PLUGIN, error));

    const auto descriptors = library.descriptors();
    REQUIRE(descriptors.size() == 1);
    CHECK(descriptors[0].id == "com.incdaw.testgain");
    CHECK(descriptors[0].name == "INCDAW Test Gain");
    CHECK(descriptors[0].vendor == "INCDAW");

    auto instance = library.create("com.incdaw.testgain", 48000.0, 512, error);
    REQUIRE(instance != nullptr);

    std::vector<float> left(256), right(256);
    for (std::size_t frame = 0; frame < 256; ++frame) {
        left[frame]  = static_cast<float>(0.5 * std::sin(0.02 * static_cast<double>(frame)));
        right[frame] = -left[frame];
    }

    const auto expectedLeft  = left;
    const auto expectedRight = right;

    REQUIRE(instance->process(left.data(), right.data(), 256));

    for (std::size_t frame = 0; frame < 256; ++frame) {
        REQUIRE(left[frame] == expectedLeft[frame] * 0.5f);
        REQUIRE(right[frame] == expectedRight[frame] * 0.5f);
    }
}

TEST_CASE("asking for a plugin the library does not have fails cleanly")
{
    plugins::ClapLibrary library;
    std::string error;

    REQUIRE(library.open(INCDAW_TESTGAIN_PLUGIN, error));
    CHECK(library.create("com.incdaw.no-such-plugin", 48000.0, 512, error) == nullptr);
    CHECK_FALSE(error.empty());
}

TEST_CASE("the out-of-process scanner reports a healthy library")
{
    const auto outcome =
        plugins::scanOutOfProcess(INCDAW_PLUGINSCAN_BINARY, INCDAW_TESTGAIN_PLUGIN);

    REQUIRE(outcome.status == plugins::ScanOutcome::Status::ok);
    REQUIRE(outcome.plugins.size() == 1);
    CHECK(outcome.plugins[0].id == "com.incdaw.testgain");
    CHECK(outcome.plugins[0].version == "0.1.0");
}

TEST_CASE("EXIT CRITERION SEED: a plugin that crashes on load kills the scanner, not the host")
{
    const auto outcome =
        plugins::scanOutOfProcess(INCDAW_PLUGINSCAN_BINARY, INCDAW_TESTCRASH_PLUGIN);

    // The child died of the plugin's segfault; this process is running this
    // assertion, which is the whole point of the isolation strategy.
    CHECK(outcome.status == plugins::ScanOutcome::Status::crashed);
    CHECK(outcome.plugins.empty());
}

TEST_CASE("a path that is not a plugin fails without crashing anyone")
{
    const auto outcome =
        plugins::scanOutOfProcess(INCDAW_PLUGINSCAN_BINARY, "/tmp/not-a-plugin.clap");

    CHECK(outcome.status == plugins::ScanOutcome::Status::failed);
}
