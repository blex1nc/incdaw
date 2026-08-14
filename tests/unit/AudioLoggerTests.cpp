// Phase 12 — the Audio Logger.
//
// A keep-newest circle: the properties that matter are that the freshest
// window comes back in order after arbitrary wrapping, that a disabled
// logger keeps nothing, and that the logging path is allocation-free.

#include "doctest.h"

#include "engine/AudioEngine.h"
#include "engine/audio/AudioLogger.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/GainNode.h"
#include "engine/dsp/SineOscillatorNode.h"

#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

/// Logs `frames` mono frames whose value encodes their global index.
void logCounting(AudioLogger& logger, FrameCount& counter, FrameCount frames)
{
    std::vector<Sample> block(static_cast<std::size_t>(frames));
    for (FrameCount frame = 0; frame < frames; ++frame)
        block[static_cast<std::size_t>(frame)] = static_cast<Sample>(counter + frame) * 1.0e-6f;

    const Sample* channels[] = {block.data()};
    logger.log(channels, 1, frames);
    counter += frames;
}

} // namespace

TEST_CASE("the logger keeps the newest window across wraps, in order")
{
    AudioLogger logger;
    logger.prepare(1000.0, 1, 1.0);   // capacity: exactly 1000 frames
    logger.setEnabled(true);

    FrameCount counter = 0;
    for (int block = 0; block < 5; ++block)
        logCounting(logger, counter, 300);   // 1500 total: wraps once

    AudioFileData grabbed;
    REQUIRE(logger.grab(grabbed) == 1000);
    REQUIRE(grabbed.frameCount == 1000);
    REQUIRE(grabbed.channelCount == 1);

    // The freshest 1000 of 1500: global frames 500..1499, oldest first.
    for (FrameCount frame = 0; frame < 1000; ++frame)
        REQUIRE(grabbed.channels[0][static_cast<std::size_t>(frame)]
                == static_cast<Sample>(500 + frame) * 1.0e-6f);
}

TEST_CASE("a partial fill grabs what exists; a disabled logger keeps nothing")
{
    AudioLogger logger;
    logger.prepare(1000.0, 1, 1.0);

    FrameCount counter = 0;

    // Disabled: nothing sticks.
    logCounting(logger, counter, 200);
    AudioFileData grabbed;
    CHECK(logger.grab(grabbed) == 0);

    logger.setEnabled(true);
    logCounting(logger, counter, 300);

    REQUIRE(logger.grab(grabbed) == 300);
    CHECK(grabbed.channels[0][0] == static_cast<Sample>(200) * 1.0e-6f);
}

TEST_CASE("the logging path is allocation-free")
{
    if (!rt::guardEnabled()) {
        MESSAGE("realtime guard not compiled in; not verified");
        return;
    }

    AudioLogger logger;
    logger.prepare(48000.0, 2, 2.0);
    logger.setEnabled(true);

    std::vector<Sample> left(512, 0.1f), right(512, -0.1f);
    const Sample* channels[] = {left.data(), right.data()};

    rt::resetViolations();

    {
        const rt::ScopedRealtimeContext realtimeScope;

        for (int block = 0; block < 64; ++block)
            logger.log(channels, 2, 512);
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
}

TEST_CASE("the engine's logger records exactly what the block rendered")
{
    AudioEngine audioEngine;

    // Headless: drive the callback interface directly, as a device would.
    incdaw::platform::AudioIOCallback& callback = audioEngine;
    callback.audioDeviceAboutToStart(48000.0, 256);

    GraphBuilder builder;
    const auto source = builder.addNode(std::make_unique<dsp::SineOscillatorNode>(440.0, 0.25f));
    const auto master = builder.addNode(std::make_unique<dsp::GainNode>(1.0f));
    builder.connect(source, master);
    builder.setMaster(master);

    auto graph = builder.compile(48000.0, 256, 1);
    REQUIRE(graph != nullptr);
    audioEngine.setGraph(std::move(graph));

    audioEngine.logger().setEnabled(true);

    std::vector<Sample> output(256);
    std::vector<Sample> heard;

    float* channels[] = {output.data()};

    for (int block = 0; block < 8; ++block) {
        callback.renderAudioBlock(channels, 1, 256, 1'000'000'000ull
                                                    + static_cast<std::uint64_t>(block));
        heard.insert(heard.end(), output.begin(), output.end());
    }

    AudioFileData grabbed;
    const auto frames = audioEngine.logger().grab(grabbed);

    REQUIRE(frames == 2048);

    for (FrameCount frame = 0; frame < frames; ++frame)
        REQUIRE(grabbed.channels[0][static_cast<std::size_t>(frame)]
                == heard[static_cast<std::size_t>(frame)]);

    audioEngine.collectRetiredGraphs();
}
