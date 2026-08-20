#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/MetronomeNode.h"
#include "engine/graph/RenderGraph.h"
#include "engine/transport/Transport.h"

#include <memory>
#include <vector>

using namespace incdaw::engine;

namespace {

std::size_t plan(Transport& transport, FrameCount blockSize, BlockSegment* out)
{
    return transport.processBlock(blockSize, out, Transport::maxSegmentsPerBlock);
}

} // namespace

TEST_CASE("a stopped transport still renders, but does not advance")
{
    // A synth releasing a note must keep being rendered after the transport
    // stops, so "stopped" cannot mean "produce no block".
    Transport transport;
    BlockSegment segments[Transport::maxSegmentsPerBlock];

    const std::size_t count = plan(transport, 128, segments);

    REQUIRE(count == 1);
    CHECK(segments[0].offset == 0);
    CHECK(segments[0].length == 128);
    CHECK(transport.position() == 0);
}

TEST_CASE("playing advances the position by exactly the block size")
{
    Transport transport;
    BlockSegment segments[Transport::maxSegmentsPerBlock];

    transport.play();

    for (int block = 1; block <= 10; ++block) {
        const std::size_t count = plan(transport, 256, segments);
        REQUIRE(count == 1);
        CHECK(transport.position() == block * 256);
    }
}

TEST_CASE("a seek takes effect at the start of the next block")
{
    Transport transport;
    BlockSegment segments[Transport::maxSegmentsPerBlock];

    transport.play();
    (void)plan(transport, 128, segments);
    REQUIRE(transport.position() == 128);

    transport.seek(48000);
    const std::size_t count = plan(transport, 128, segments);

    REQUIRE(count == 1);
    CHECK(segments[0].startFrame == 48000);
    CHECK(transport.position() == 48000 + 128);
}

TEST_CASE("a negative seek is clamped to the song start")
{
    Transport transport;
    transport.seek(-5000);

    BlockSegment segments[Transport::maxSegmentsPerBlock];
    transport.play();
    (void)plan(transport, 64, segments);

    CHECK(segments[0].startFrame == 0);
}

TEST_CASE("pause holds position and stop returns to the start")
{
    Transport transport;
    BlockSegment segments[Transport::maxSegmentsPerBlock];

    transport.play();
    (void)plan(transport, 1000, segments);

    transport.pause();
    (void)plan(transport, 1000, segments);
    CHECK(transport.position() == 1000);

    transport.stop();
    (void)plan(transport, 1000, segments);
    CHECK(transport.position() == 0);
}

TEST_CASE("a block that crosses the loop end is split at the exact frame")
{
    Transport transport;
    BlockSegment segments[Transport::maxSegmentsPerBlock];

    transport.setLoopRange(0, 1000);
    transport.setLoopEnabled(true);
    transport.seek(900);
    transport.play();

    const std::size_t count = plan(transport, 256, segments);

    REQUIRE(count == 2);

    // Up to the loop end...
    CHECK(segments[0].offset == 0);
    CHECK(segments[0].length == 100);
    CHECK(segments[0].startFrame == 900);
    CHECK_FALSE(segments[0].startsAfterLoopWrap);

    // ...then from the loop start, for the remainder of the block.
    CHECK(segments[1].offset == 100);
    CHECK(segments[1].length == 156);
    CHECK(segments[1].startFrame == 0);
    CHECK(segments[1].startsAfterLoopWrap);

    CHECK(transport.position() == 156);
}

TEST_CASE("the segments of a block always cover it exactly once")
{
    // Whatever the loop geometry, the plan must tile the block: any gap is
    // silence, any overlap is a double-rendered sample.
    Transport transport;
    BlockSegment segments[Transport::maxSegmentsPerBlock];

    transport.setLoopRange(1000, 1700);
    transport.setLoopEnabled(true);
    transport.play();
    transport.seek(1500);

    for (int block = 0; block < 200; ++block) {
        const std::size_t count = plan(transport, 512, segments);
        REQUIRE(count >= 1);

        FrameCount covered = 0;
        for (std::size_t index = 0; index < count; ++index) {
            CHECK(segments[index].offset == covered);
            CHECK(segments[index].length > 0);
            covered += segments[index].length;
        }

        CHECK(covered == 512);
    }
}

TEST_CASE("playback stays inside the loop once it has entered it")
{
    Transport transport;
    BlockSegment segments[Transport::maxSegmentsPerBlock];

    transport.setLoopRange(2000, 5000);
    transport.setLoopEnabled(true);
    transport.seek(2000);
    transport.play();

    for (int block = 0; block < 500; ++block) {
        const std::size_t count = plan(transport, 333, segments);
        for (std::size_t index = 0; index < count; ++index) {
            CHECK(segments[index].startFrame >= 2000);
            CHECK(segments[index].startFrame < 5000);
        }
    }
}

TEST_CASE("a seek outside the loop plays on from there rather than snapping back")
{
    Transport transport;
    BlockSegment segments[Transport::maxSegmentsPerBlock];

    transport.setLoopRange(1000, 2000);
    transport.setLoopEnabled(true);
    transport.seek(10000);
    transport.play();

    const std::size_t count = plan(transport, 128, segments);
    REQUIRE(count == 1);
    CHECK(segments[0].startFrame == 10000);
    CHECK(transport.position() == 10128);
}

TEST_CASE("an inverted loop range is rejected rather than silently reinterpreted")
{
    Transport transport;
    transport.setLoopRange(1000, 2000);
    transport.setLoopRange(5000, 4000);   // invalid: end before start

    CHECK(transport.loopStart() == 1000);
    CHECK(transport.loopEnd() == 2000);
}

TEST_CASE("planning a block is realtime-safe")
{
    Transport transport;
    BlockSegment segments[Transport::maxSegmentsPerBlock];

    transport.setLoopRange(0, 777);
    transport.setLoopEnabled(true);
    transport.play();

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        for (int block = 0; block < 5000; ++block)
            (void)plan(transport, 128, segments);
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
}

// ── Phase 3 exit criterion ────────────────────────────────────────────────────

namespace {

/// Renders the metronome through a real graph and collects the absolute frame
/// of every click onset — the same path the audio callback takes.
std::vector<FramePosition> renderClickFrames(Transport& transport, FrameCount blockSize, int blockCount)
{
    auto  metronome = std::make_unique<dsp::MetronomeNode>(transport.tempoMap());
    auto* node      = metronome.get();

    GraphBuilder builder;
    const auto index = builder.addNode(std::move(metronome));
    builder.setMaster(index);

    auto graph = builder.compile(transport.tempoMap().sampleRate(), blockSize, 1);
    REQUIRE(graph != nullptr);

    AudioBufferPool output;
    output.allocate(1, 1, blockSize);

    std::vector<FramePosition> clicks;
    BlockSegment               segments[Transport::maxSegmentsPerBlock];

    for (int block = 0; block < blockCount; ++block) {
        const std::size_t count = transport.processBlock(blockSize, segments,
                                                         Transport::maxSegmentsPerBlock);

        for (std::size_t segment = 0; segment < count; ++segment) {
            const BlockSegment& part = segments[segment];
            if (part.length <= 0)
                continue;

            // The engine hands the graph the transport's state with every
            // block; the metronome reads THAT rather than reaching back into
            // the transport, so the harness has to do the same.
            graph->process(output.buffer(0).subBlock(part.offset, part.length),
                           part.length, part.startFrame, nullptr, transport.isPlaying());

            for (std::size_t click = 0; click < node->lastBlockClickCount(); ++click)
                clicks.push_back(part.startFrame + node->lastBlockClickOffsets()[click]);
        }
    }

    return clicks;
}

} // namespace

TEST_CASE("click events land on the exact frame at a constant tempo")
{
    constexpr SampleRate rate      = 48000.0;
    constexpr FrameCount blockSize = 128;

    Transport transport{TempoMap{120.0, rate}};
    transport.play();

    // 4 seconds at 120 BPM = 8 beats.
    const auto clicks = renderClickFrames(transport, blockSize, 4 * 48000 / 128);

    REQUIRE(clicks.size() == 8);

    // At 120 BPM a beat is 24000 frames.
    for (std::size_t beat = 0; beat < clicks.size(); ++beat)
        CHECK(clicks[beat] == static_cast<FramePosition>(beat) * 24000);
}

TEST_CASE("click events stay sample-accurate across a tempo change")
{
    constexpr SampleRate rate      = 48000.0;
    constexpr FrameCount blockSize = 128;

    // 120 BPM for one bar, then 180 BPM.
    TempoMap map{120.0, rate};
    map.setTempoEvents({{0, 120.0}, {ticksPerQuarterNote * 4, 180.0}});

    Transport transport{map};
    transport.play();

    const auto clicks = renderClickFrames(transport, blockSize, 6 * 48000 / 128);

    REQUIRE(clicks.size() >= 8);

    // Every click must sit exactly where the tempo map says its beat is — the
    // block boundaries must have had no influence whatsoever.
    for (std::size_t beat = 0; beat < clicks.size(); ++beat) {
        const Tick beatTick = static_cast<Tick>(beat) * ticksPerQuarterNote;
        CHECK(clicks[beat] == map.frameForTick(beatTick));
    }

    // Sanity: the beats after the change really are shorter.
    CHECK(clicks[1] - clicks[0] == 24000);            // 120 BPM
    CHECK(clicks[5] - clicks[4] == 16000);            // 180 BPM
}

TEST_CASE("click events stay sample-accurate across a loop boundary")
{
    constexpr SampleRate rate      = 48000.0;
    constexpr FrameCount blockSize = 128;

    const TempoMap map{120.0, rate};

    Transport transport{map};

    // Loop exactly one bar: 4 beats at 120 BPM = 96000 frames.
    transport.setLoopRange(0, 96000);
    transport.setLoopEnabled(true);
    transport.play();

    // Three passes through the loop.
    const auto clicks = renderClickFrames(transport, blockSize, 3 * 96000 / 128);

    REQUIRE(clicks.size() == 12);

    // Each pass must produce the identical four click positions. If the loop
    // wrap were applied at a block boundary instead of its exact frame, the
    // second and third passes would drift.
    for (int pass = 0; pass < 3; ++pass)
        for (int beat = 0; beat < 4; ++beat)
            CHECK(clicks[static_cast<std::size_t>(pass * 4 + beat)] == beat * 24000);
}

TEST_CASE("click accuracy does not depend on the block size")
{
    // The strongest form of the criterion: change the buffer size and the
    // musical result must be bit-for-bit identical in timing.
    constexpr SampleRate rate = 48000.0;

    std::vector<FramePosition> reference;

    for (const FrameCount blockSize : {32, 64, 128, 256, 480, 512, 1024}) {
        TempoMap map{132.0, rate};
        map.setTempoEvents({{0, 132.0}, {ticksPerQuarterNote * 5, 84.0}});

        Transport transport{map};
        transport.play();

        const int blocks = static_cast<int>(8 * 48000 / blockSize);
        const auto clicks = renderClickFrames(transport, blockSize, blocks);

        REQUIRE(clicks.size() > 8);

        if (reference.empty())
            reference = clicks;
        else
            for (std::size_t index = 0; index < 8; ++index)
                CHECK(clicks[index] == reference[index]);
    }
}

TEST_CASE("the metronome is silent when the transport is stopped")
{
    Transport transport{TempoMap{120.0, 48000.0}};
    // deliberately not started

    const auto clicks = renderClickFrames(transport, 128, 400);
    CHECK(clicks.empty());
}
