// Phase 12 — input monitoring.
//
// The monitor node bridges two clock domains through a ring; the properties
// that matter are pass-through fidelity, the drift cap (latency must not
// grow without bound), honest silence on underrun, and an allocation-free
// read path.

#include "doctest.h"

#include "engine/audio/InputMonitorNode.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "project/Model.h"
#include "project/ProjectGraphCompiler.h"

#include <cmath>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

Sample tone(FrameCount frame) { return static_cast<Sample>(0.3 * std::sin(0.05 * static_cast<double>(frame))); }

std::vector<Sample> renderBlock(InputMonitorNode& node, FrameCount blockSize,
                                std::size_t channels = 2, std::size_t channel = 0)
{
    AudioBufferPool pool;
    pool.allocate(1, channels, blockSize);
    pool.buffer(0).clear();

    ProcessContext context;
    context.output     = pool.buffer(0);
    context.frameCount = blockSize;
    context.sampleRate = 48000.0;

    node.process(context);

    std::vector<Sample> out(static_cast<std::size_t>(blockSize));
    for (FrameCount frame = 0; frame < blockSize; ++frame)
        out[static_cast<std::size_t>(frame)] = pool.buffer(0).channel(channel)[frame];

    return out;
}

} // namespace

TEST_CASE("mono input passes through onto every output channel")
{
    SampleRingBuffer ring;
    ring.reset(4096);

    InputMonitorNode node{&ring, 1};
    node.prepare(48000.0, 256);

    std::vector<Sample> input(128);
    for (FrameCount frame = 0; frame < 128; ++frame)
        input[static_cast<std::size_t>(frame)] = tone(frame);

    REQUIRE(ring.write(input.data(), input.size()) == input.size());

    const auto left  = renderBlock(node, 128, 2, 0);
    // The ring is drained; render the right channel from a fresh write.
    REQUIRE(ring.write(input.data(), input.size()) == input.size());
    const auto right = renderBlock(node, 128, 2, 1);

    for (FrameCount frame = 0; frame < 128; ++frame) {
        REQUIRE(left[static_cast<std::size_t>(frame)] == tone(frame));
        REQUIRE(right[static_cast<std::size_t>(frame)] == tone(frame));
    }
}

TEST_CASE("an empty ring plays silence, not a wait")
{
    SampleRingBuffer ring;
    ring.reset(4096);

    InputMonitorNode node{&ring, 2};
    node.prepare(48000.0, 256);

    const auto out = renderBlock(node, 128);

    for (const Sample value : out)
        REQUIRE(value == 0.0f);
}

TEST_CASE("a backlog past the drift cap is skipped, and the newest audio plays")
{
    SampleRingBuffer ring;
    ring.reset(1 << 15);

    InputMonitorNode node{&ring, 1};
    node.prepare(48000.0, 128);

    // Ten blocks of backlog, values encoding their block index.
    for (int block = 0; block < 10; ++block) {
        std::vector<Sample> chunk(128, static_cast<Sample>(block) * 0.01f);
        REQUIRE(ring.write(chunk.data(), chunk.size()) == chunk.size());
    }

    const auto out = renderBlock(node, 128, 2, 0);

    // The cap discards down to at most a few blocks: what plays must come
    // from the back half of the backlog, and the ring must be nearly caught
    // up afterwards.
    CHECK(out[0] >= 0.049f);
    CHECK(ring.size() <= 128 * 5);
}

TEST_CASE("the monitor path is allocation-free")
{
    if (!rt::guardEnabled()) {
        MESSAGE("realtime guard not compiled in; not verified");
        return;
    }

    SampleRingBuffer ring;
    ring.reset(4096);

    InputMonitorNode node{&ring, 2};
    node.prepare(48000.0, 256);

    std::vector<Sample> chunk(512, 0.25f);

    AudioBufferPool pool;
    pool.allocate(1, 2, 256);

    rt::resetViolations();

    {
        const rt::ScopedRealtimeContext realtimeScope;

        for (int block = 0; block < 16; ++block) {
            (void)ring.write(chunk.data(), chunk.size());   // the capture side

            ProcessContext context;
            context.output     = pool.buffer(0);
            context.frameCount = 256;
            context.sampleRate = 48000.0;
            pool.buffer(0).clear();
            node.process(context);
        }
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
}

TEST_CASE("the compiler wires a monitor node to the master when asked")
{
    SampleRingBuffer ring;
    ring.reset(4096);

    std::vector<Sample> input(256);
    for (FrameCount frame = 0; frame < 256; ++frame)
        input[static_cast<std::size_t>(frame)] = tone(frame);
    REQUIRE(ring.write(input.data(), input.size()) == input.size());

    project::Project projectModel;
    projectModel.tempoMap().setSampleRate(48000.0);

    project::GraphCompileOptions options;
    options.source              = project::PlaybackSource::arrangement;
    options.masterGain          = 1.0f;
    options.monitorRing         = &ring;
    options.monitorChannelCount = 1;

    auto compiled = project::compileProjectGraph(projectModel, projectModel.tempoMap(), options);
    REQUIRE(bool(compiled));

    AudioBufferPool pool;
    pool.allocate(1, 2, 256);
    pool.buffer(0).clear();
    compiled.graph->process(pool.buffer(0), 256, 0);

    // Through the master strip: centre pan scales both channels equally.
    const Sample scale = pool.buffer(0).channel(0)[10] / tone(10);
    CHECK(scale > 0.5f);

    for (FrameCount frame = 0; frame < 256; ++frame)
        REQUIRE(std::abs(pool.buffer(0).channel(0)[frame] - tone(frame) * scale) < 1.0e-6f);
}
