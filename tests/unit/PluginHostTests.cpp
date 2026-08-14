// Phase 13 (part 1) — CLAP hosting.
//
// Tested against the suite's OWN plugins: a well-behaved gain and a hostile
// one that segfaults on load. The load-bearing property is the isolation
// contract: the hostile plugin kills the SCANNER, and this process — the
// host — proves it survived by finishing the test.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/GainNode.h"
#include "engine/dsp/SineOscillatorNode.h"
#include "engine/graph/RenderGraph.h"
#include "plugins/PluginNode.h"
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

TEST_CASE("a hosted plugin processes inside the render graph, allocation-free")
{
    using namespace incdaw::engine;

    plugins::ClapLibrary library;
    std::string error;
    REQUIRE(library.open(INCDAW_TESTGAIN_PLUGIN, error));

    auto instance = library.create("com.incdaw.testgain", 48000.0, 256, error);
    REQUIRE(instance != nullptr);

    // sine -> plugin insert -> master. What reaches the master must be the
    // sine through the plugin's -6 dB, sample for sample.
    GraphBuilder builder;
    const auto source = builder.addNode(std::make_unique<dsp::SineOscillatorNode>(220.0, 0.4f));
    const auto insert = builder.addNode(
        std::make_unique<plugins::PluginNode>(std::move(instance)));
    const auto master = builder.addNode(std::make_unique<dsp::GainNode>(1.0f));
    builder.connect(source, insert);
    builder.connect(insert, master);
    builder.setMaster(master);

    auto withPlugin = builder.compile(48000.0, 256, 2);
    REQUIRE(withPlugin != nullptr);

    GraphBuilder reference;
    const auto referenceSource =
        reference.addNode(std::make_unique<dsp::SineOscillatorNode>(220.0, 0.4f));
    const auto referenceMaster = reference.addNode(std::make_unique<dsp::GainNode>(0.5f));
    reference.connect(referenceSource, referenceMaster);
    reference.setMaster(referenceMaster);

    auto expected = reference.compile(48000.0, 256, 2);
    REQUIRE(expected != nullptr);

    AudioBufferPool pool;
    pool.allocate(2, 2, 256);

    rt::resetViolations();

    for (int block = 0; block < 8; ++block) {
        pool.buffer(0).clear();
        pool.buffer(1).clear();

        {
            const rt::ScopedRealtimeContext realtimeScope;
            withPlugin->process(pool.buffer(0), 256,
                                static_cast<FramePosition>(block) * 256);
        }

        expected->process(pool.buffer(1), 256, static_cast<FramePosition>(block) * 256);

        for (std::size_t channel = 0; channel < 2; ++channel)
            for (FrameCount frame = 0; frame < 256; ++frame)
                REQUIRE(pool.buffer(0).channel(channel)[frame]
                        == pool.buffer(1).channel(channel)[frame]);
    }

    if (rt::guardEnabled()) {
        CHECK(rt::allocationViolations() == 0);
        CHECK(rt::deallocationViolations() == 0);
    }
}

TEST_CASE("a path that is not a plugin fails without crashing anyone")
{
    const auto outcome =
        plugins::scanOutOfProcess(INCDAW_PLUGINSCAN_BINARY, "/tmp/not-a-plugin.clap");

    CHECK(outcome.status == plugins::ScanOutcome::Status::failed);
}
