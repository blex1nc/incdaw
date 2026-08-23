// TRACK B (B12, increment 2) — the performance scheduler, headless.
//
// docs/PERFORMANCE_MODE.md §7 puts this first on purpose: it is where the
// design is proved or discarded, before any audio node depends on it. The
// scheduler knows about slots, clips, frames and behaviours and nothing about
// audio, so every property below is asserted by feeding it triggers and asking
// what is sounding.
//
// Three properties carry the design:
//
//   * a trigger takes effect on its quantised frame, not at the top of the
//     block that contains it — and always FORWARD, because a quantised trigger
//     playing early is the one failure a player hears immediately;
//   * every press and motion behaviour is the same state machine with a
//     different branch, so each is one case here rather than one code path;
//   * `advanceTo` allocates nothing and takes no lock, which is what makes the
//     whole approach viable at all.

#include "doctest.h"

#include "engine/core/RealtimeGuard.h"
#include "engine/performance/PerformanceScheduler.h"
#include "engine/transport/TempoMap.h"

#include <vector>

using namespace incdaw;
using engine::FrameCount;
using engine::FramePosition;
using engine::PerformanceMotion;
using engine::PerformancePress;
using engine::PerformanceScheduler;
using engine::Tick;
using engine::ticksPerQuarterNote;

namespace {

/// 120 bpm at 48 kHz: a quarter note is 24,000 frames and a bar is 96,000.
engine::TempoMap makeTempo()
{
    engine::TempoMap map;
    map.setSampleRate(48000.0);
    return map;
}

constexpr FrameCount quarter = 24000;
constexpr FrameCount barFrames = quarter * 4;

/// One track of `clips` clips, each `length` frames long, laid end to end.
PerformanceScheduler::TrackSetup track(std::size_t clips, FrameCount length,
                                       PerformancePress press = PerformancePress::retrigger,
                                       PerformanceMotion motion = PerformanceMotion::stay,
                                       Tick syncTicks = 0)
{
    PerformanceScheduler::TrackSetup setup;
    setup.press     = press;
    setup.motion    = motion;
    setup.syncTicks = syncTicks;

    for (std::size_t index = 0; index < clips; ++index)
        setup.clips.push_back({static_cast<FramePosition>(index) * length, length});

    return setup;
}

/// Walks the scheduler forward the way an audio callback would, in blocks.
void run(PerformanceScheduler& scheduler, const engine::TempoMap& tempo,
         FramePosition from, FramePosition to, FrameCount blockSize = 512)
{
    for (FramePosition frame = from; frame < to; frame += blockSize)
        scheduler.advanceTo(frame, tempo);
}

} // namespace

// ── The table ────────────────────────────────────────────────────────────────

TEST_CASE("an untriggered track sounds nothing")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(4, barFrames)});

    CHECK(scheduler.trackCount() == 1);

    scheduler.advanceTo(0, tempo);
    CHECK_FALSE(scheduler.voiceAt(0, 0).sounding);
    CHECK_FALSE(scheduler.voiceAt(0, barFrames * 2).sounding);

    // And a track that does not exist is silent rather than an error.
    CHECK_FALSE(scheduler.voiceAt(9, 0).sounding);
    CHECK(scheduler.postTrigger(9, 0, true, 0));
}

TEST_CASE("a pressed clip sounds from its trigger frame, and reports how far into itself it is")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(2, barFrames)});

    REQUIRE(scheduler.postTrigger(0, 1, true, 1000));
    scheduler.advanceTo(1000, tempo);

    const auto voice = scheduler.voiceAt(0, 1000);
    REQUIRE(voice.sounding);
    CHECK(voice.clip == 1);
    CHECK(voice.sourceFrame == 0);

    CHECK(scheduler.voiceAt(0, 1000 + 5000).sourceFrame == 5000);

    // Nothing before it started, and nothing after it ran out.
    CHECK_FALSE(scheduler.voiceAt(0, 999).sounding);
    CHECK_FALSE(scheduler.voiceAt(0, 1000 + barFrames).sounding);
}

// ── Quantisation ─────────────────────────────────────────────────────────────

TEST_CASE("a trigger lands on the bar line, not at the top of the block containing it")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(1, barFrames * 4, PerformancePress::retrigger,
                             PerformanceMotion::stay, ticksPerQuarterNote * 4)});

    // Hit 2,000 frames — about 40 ms — before the second bar line.
    const FramePosition barLine = barFrames;
    REQUIRE(scheduler.postTrigger(0, 0, true, barLine - 2000));

    // The block containing the press must not start it: the clip belongs on
    // the bar, and the press was early.
    scheduler.advanceTo(barLine - 2000, tempo);
    CHECK_FALSE(scheduler.voiceAt(0, barLine - 2000).sounding);
    CHECK_FALSE(scheduler.voiceAt(0, barLine - 1).sounding);

    // The scheduler says where the block should be split, which is the frame
    // itself and not the next block boundary.
    CHECK(scheduler.nextBoundaryIn(barLine - 2000, 512) == barLine - 2000 + 512);
    CHECK(scheduler.nextBoundaryIn(barLine - 100, 512) == barLine);

    scheduler.advanceTo(barLine, tempo);

    const auto voice = scheduler.voiceAt(0, barLine);
    REQUIRE(voice.sounding);
    CHECK(voice.sourceFrame == 0);
}

TEST_CASE("quantisation rounds forward, never back")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(1, barFrames * 4, PerformancePress::retrigger,
                             PerformanceMotion::stay, ticksPerQuarterNote * 4)});

    // Hit just AFTER the bar line: the answer is the next bar, not the one
    // that has already gone by. Playing early is the failure a player hears.
    REQUIRE(scheduler.postTrigger(0, 0, true, barFrames + 500));

    scheduler.advanceTo(barFrames + 500, tempo);
    CHECK_FALSE(scheduler.voiceAt(0, barFrames + 500).sounding);

    run(scheduler, tempo, barFrames + 500, barFrames * 2 + 1024);
    CHECK(scheduler.voiceAt(0, barFrames * 2).sounding);
}

TEST_CASE("a trigger exactly on the grid is not pushed to the next one")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(1, barFrames * 4, PerformancePress::retrigger,
                             PerformanceMotion::stay, ticksPerQuarterNote)});

    REQUIRE(scheduler.postTrigger(0, 0, true, quarter));
    scheduler.advanceTo(quarter, tempo);

    CHECK(scheduler.voiceAt(0, quarter).sounding);
}

TEST_CASE("sync off starts the clip on the frame the pad was hit")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(1, barFrames)});

    REQUIRE(scheduler.postTrigger(0, 0, true, 777));
    scheduler.advanceTo(777, tempo);

    CHECK(scheduler.voiceAt(0, 777).sounding);
}

TEST_CASE("the last press before the trigger lands is the one that counts")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(3, barFrames, PerformancePress::retrigger,
                             PerformanceMotion::stay, ticksPerQuarterNote * 4)});

    REQUIRE(scheduler.postTrigger(0, 0, true, 1000));
    REQUIRE(scheduler.postTrigger(0, 2, true, 2000));

    run(scheduler, tempo, 0, barFrames + 1024);

    const auto voice = scheduler.voiceAt(0, barFrames);
    REQUIRE(voice.sounding);
    CHECK(voice.clip == 2);
}

// ── Press behaviours ─────────────────────────────────────────────────────────

TEST_CASE("retrigger restarts on a second press and ignores the release")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(1, barFrames)});

    REQUIRE(scheduler.postTrigger(0, 0, true, 0));
    scheduler.advanceTo(0, tempo);
    CHECK(scheduler.voiceAt(0, 5000).sourceFrame == 5000);

    REQUIRE(scheduler.postTrigger(0, 0, false, 6000));
    scheduler.advanceTo(6000, tempo);
    CHECK(scheduler.voiceAt(0, 6000).sounding);   // the release means nothing

    REQUIRE(scheduler.postTrigger(0, 0, true, 8000));
    scheduler.advanceTo(8000, tempo);
    CHECK(scheduler.voiceAt(0, 8000).sourceFrame == 0);   // back to the top
}

TEST_CASE("hold sounds while the pad is down")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(1, barFrames * 8, PerformancePress::hold)});

    REQUIRE(scheduler.postTrigger(0, 0, true, 0));
    scheduler.advanceTo(0, tempo);
    CHECK(scheduler.voiceAt(0, 10000).sounding);

    REQUIRE(scheduler.postTrigger(0, 0, false, 20000));
    scheduler.advanceTo(20000, tempo);
    CHECK_FALSE(scheduler.voiceAt(0, 20000).sounding);
}

TEST_CASE("latch stops on the second press of the same pad")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(2, barFrames * 8, PerformancePress::latch)});

    REQUIRE(scheduler.postTrigger(0, 0, true, 0));
    scheduler.advanceTo(0, tempo);
    CHECK(scheduler.voiceAt(0, 5000).sounding);

    // Releasing changes nothing, which is what latch means.
    REQUIRE(scheduler.postTrigger(0, 0, false, 6000));
    scheduler.advanceTo(6000, tempo);
    CHECK(scheduler.voiceAt(0, 6000).sounding);

    REQUIRE(scheduler.postTrigger(0, 0, true, 9000));
    scheduler.advanceTo(9000, tempo);
    CHECK_FALSE(scheduler.voiceAt(0, 9000).sounding);

    // A DIFFERENT pad switches rather than stops.
    REQUIRE(scheduler.postTrigger(0, 0, true, 10000));
    scheduler.advanceTo(10000, tempo);
    REQUIRE(scheduler.postTrigger(0, 1, true, 11000));
    scheduler.advanceTo(11000, tempo);

    const auto voice = scheduler.voiceAt(0, 11000);
    REQUIRE(voice.sounding);
    CHECK(voice.clip == 1);
}

TEST_CASE("hold-and-motion advances on the release, hold-and-stop does not")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({
        track(3, barFrames * 8, PerformancePress::holdAndMotion,
              PerformanceMotion::marchAndWrap),
        track(3, barFrames * 8, PerformancePress::holdAndStop,
              PerformanceMotion::marchAndWrap),
    });

    for (std::size_t index = 0; index < 2; ++index) {
        REQUIRE(scheduler.postTrigger(index, 0, true, 0));
        REQUIRE(scheduler.postTrigger(index, 0, false, 5000));
    }

    scheduler.advanceTo(0, tempo);
    scheduler.advanceTo(5000, tempo);

    const auto motion = scheduler.voiceAt(0, 5000);
    REQUIRE(motion.sounding);
    CHECK(motion.clip == 1);          // the release handed over

    CHECK_FALSE(scheduler.voiceAt(1, 5000).sounding);   // the release stopped it
}

// ── Motion behaviours ────────────────────────────────────────────────────────

TEST_CASE("stay repeats the same clip when it runs out")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(3, quarter, PerformancePress::retrigger,
                             PerformanceMotion::stay)});

    REQUIRE(scheduler.postTrigger(0, 1, true, 0));
    run(scheduler, tempo, 0, quarter * 3);

    const auto voice = scheduler.voiceAt(0, quarter * 2 + 100);
    REQUIRE(voice.sounding);
    CHECK(voice.clip == 1);
}

TEST_CASE("one-shot plays once and stops")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(3, quarter, PerformancePress::retrigger,
                             PerformanceMotion::oneShot)});

    REQUIRE(scheduler.postTrigger(0, 0, true, 0));
    run(scheduler, tempo, 0, quarter * 2);

    CHECK_FALSE(scheduler.voiceAt(0, quarter + 100).sounding);
}

TEST_CASE("march-and-wrap walks the clips and comes back round")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(3, quarter, PerformancePress::retrigger,
                             PerformanceMotion::marchAndWrap)});

    REQUIRE(scheduler.postTrigger(0, 0, true, 0));

    const std::size_t expected[] = {0, 1, 2, 0, 1};

    for (std::size_t step = 0; step < 5; ++step) {
        const auto at = static_cast<FramePosition>(step) * quarter + 100;
        run(scheduler, tempo, at - 100, at + 1);

        const auto voice = scheduler.voiceAt(0, at);
        REQUIRE(voice.sounding);
        CHECK(voice.clip == expected[step]);
    }
}

TEST_CASE("march-and-stay holds on the last clip, march-and-stop falls silent after it")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({
        track(2, quarter, PerformancePress::retrigger, PerformanceMotion::marchAndStay),
        track(2, quarter, PerformancePress::retrigger, PerformanceMotion::marchAndStop),
    });

    REQUIRE(scheduler.postTrigger(0, 0, true, 0));
    REQUIRE(scheduler.postTrigger(1, 0, true, 0));

    run(scheduler, tempo, 0, quarter * 4);

    const auto stay = scheduler.voiceAt(0, quarter * 3 + 100);
    REQUIRE(stay.sounding);
    CHECK(stay.clip == 1);

    CHECK_FALSE(scheduler.voiceAt(1, quarter * 3 + 100).sounding);
}

TEST_CASE("random is deterministic from the seed, and exclusive random never repeats")
{
    const engine::TempoMap tempo = makeTempo();

    const auto sequence = [&tempo](PerformanceMotion motion, std::uint64_t seed) {
        PerformanceScheduler scheduler;
        scheduler.prepare({track(4, quarter, PerformancePress::retrigger, motion)});
        scheduler.setRandomSeed(seed);

        REQUIRE(scheduler.postTrigger(0, 0, true, 0));

        std::vector<std::size_t> played;

        for (std::size_t step = 0; step < 8; ++step) {
            const auto at = static_cast<FramePosition>(step) * quarter + 100;
            run(scheduler, tempo, at - 100, at + 1);

            const auto voice = scheduler.voiceAt(0, at);
            REQUIRE(voice.sounding);
            played.push_back(voice.clip);
        }

        return played;
    };

    // The same seed is the same performance, which is what makes rendering one
    // offline possible at all.
    CHECK(sequence(PerformanceMotion::random, 12345)
          == sequence(PerformanceMotion::random, 12345));

    CHECK(sequence(PerformanceMotion::random, 12345)
          != sequence(PerformanceMotion::random, 999));

    const auto exclusive = sequence(PerformanceMotion::exclusiveRandom, 4242);
    for (std::size_t index = 1; index < exclusive.size(); ++index)
        CHECK(exclusive[index] != exclusive[index - 1]);
}

// ── Position sync ────────────────────────────────────────────────────────────

TEST_CASE("position sync joins a clip in phase rather than from its start")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler::TrackSetup setup =
        track(2, barFrames, PerformancePress::retrigger, PerformanceMotion::stay);
    setup.positionSync = true;

    PerformanceScheduler scheduler;
    scheduler.prepare({setup});

    // Clip 1 sits at bar 2 on the timeline; triggering it a beat into that bar
    // must join it a beat in.
    REQUIRE(scheduler.postTrigger(0, 1, true, barFrames + quarter));
    scheduler.advanceTo(barFrames + quarter, tempo);

    const auto voice = scheduler.voiceAt(0, barFrames + quarter);
    REQUIRE(voice.sounding);
    CHECK(voice.clip == 1);
    CHECK(voice.sourceFrame == quarter);

    // And it still ends where the clip ends, not a beat late: joined a beat
    // in, it has a bar minus a beat left to run. The scheduler is a live
    // table rather than a history, so this is asked while walking forward
    // — querying a frame the scheduler has already passed is not a question
    // it can answer.
    run(scheduler, tempo, barFrames + quarter, barFrames * 2 - 600);

    const auto late = scheduler.voiceAt(0, barFrames * 2 - 600);
    REQUIRE(late.sounding);
    CHECK(late.clip == 1);
    CHECK(late.sourceFrame == barFrames - 600);
}

// ── Housekeeping ─────────────────────────────────────────────────────────────

TEST_CASE("reset silences everything and drops what was pending")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({track(2, barFrames)});

    REQUIRE(scheduler.postTrigger(0, 0, true, 0));
    scheduler.advanceTo(0, tempo);
    REQUIRE(scheduler.voiceAt(0, 100).sounding);

    REQUIRE(scheduler.postTrigger(0, 1, true, 50000));
    scheduler.reset();

    scheduler.advanceTo(60000, tempo);
    CHECK_FALSE(scheduler.voiceAt(0, 60000).sounding);
}

TEST_CASE("a full trigger ring drops the oldest and says so")
{
    PerformanceScheduler scheduler;
    scheduler.prepare({track(1, barFrames)});

    for (std::size_t index = 0; index < PerformanceScheduler::triggerCapacity; ++index)
        CHECK(scheduler.postTrigger(0, 0, true, static_cast<FramePosition>(index)));

    // One past capacity: the trigger is still queued, and the caller is told
    // that an older one went — a dropped pad hit is recoverable, a blocked
    // audio thread is not.
    CHECK_FALSE(scheduler.postTrigger(0, 0, true, 999999));
}

TEST_CASE("a zero-length clip cannot spin the scheduler")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler::TrackSetup setup;
    setup.motion = PerformanceMotion::marchAndWrap;
    setup.clips  = {{0, 0}, {0, 0}};

    PerformanceScheduler scheduler;
    scheduler.prepare({setup});

    REQUIRE(scheduler.postTrigger(0, 0, true, 0));

    // If a zero-length clip could hand over inside one call, this would not
    // return. One handover per call is what bounds it.
    run(scheduler, tempo, 0, quarter);
    CHECK(scheduler.trackCount() == 1);
}

// ── The realtime contract ────────────────────────────────────────────────────

TEST_CASE("advancing a block allocates nothing and takes no lock")
{
    const engine::TempoMap tempo = makeTempo();

    PerformanceScheduler scheduler;
    scheduler.prepare({
        track(8, quarter, PerformancePress::retrigger, PerformanceMotion::marchAndWrap,
              ticksPerQuarterNote),
        track(8, quarter, PerformancePress::hold, PerformanceMotion::random,
              ticksPerQuarterNote * 4),
        track(8, quarter, PerformancePress::latch, PerformanceMotion::exclusiveRandom),
    });

    for (std::size_t index = 0; index < 3; ++index)
        REQUIRE(scheduler.postTrigger(index, index, true, 0));

    engine::rt::resetViolations();

    {
        const engine::rt::ScopedRealtimeContext section;

        for (FramePosition frame = 0; frame < quarter * 40; frame += 512) {
            scheduler.advanceTo(frame, tempo);
            (void)scheduler.nextBoundaryIn(frame, 512);

            for (std::size_t index = 0; index < 3; ++index)
                (void)scheduler.voiceAt(index, frame);
        }
    }

    CHECK(engine::rt::allocationViolations() == 0);
    CHECK(engine::rt::deallocationViolations() == 0);
}

// ── TRACK B (B12, increment 3) — the engine path ─────────────────────────────
//
// AudioClipNode gains an optional scheduler, whose absence is exactly the
// behaviour every existing project has. With one set, the node stops playing
// its clips at their timeline positions and plays whichever the table says is
// triggered — splitting its own block at the table's boundaries, which is what
// turns "quantised" from a frame number into something a listener hears.

#include "engine/audio/AudioClipNode.h"
#include "engine/core/AudioBufferPool.h"

namespace {

std::shared_ptr<engine::AudioFileData> flat(FrameCount frames, engine::Sample level)
{
    auto data = std::make_shared<engine::AudioFileData>();
    data->sampleRate   = 48000.0;
    data->channelCount = 1;
    data->frameCount   = frames;
    data->channels.assign(1, std::vector<engine::Sample>(
                                 static_cast<std::size_t>(frames), level));
    return data;
}

/// A node holding two flat clips at distinguishable levels, under a scheduler.
struct NodeFixture {
    PerformanceScheduler  scheduler;
    engine::AudioClipNode node;

    NodeFixture(PerformanceScheduler::TrackSetup setup)
    {
        scheduler.prepare({std::move(setup)});

        for (int index = 0; index < 2; ++index) {
            engine::AudioClipNode::PlacedClip clip;
            clip.audio  = flat(barFrames * 4, index == 0 ? 0.25f : 0.75f);
            clip.start  = static_cast<FramePosition>(index) * barFrames;
            clip.length = barFrames * 4;
            node.addClip(std::move(clip));
        }

        node.prepare(48000.0, 512);
        node.setPerformance(&scheduler, 0, makeTempo());
    }

    /// Renders one block and returns channel 0.
    std::vector<engine::Sample> render(FramePosition at, FrameCount blockSize = 512)
    {
        engine::AudioBufferPool pool;
        pool.allocate(1, 2, blockSize);
        pool.buffer(0).clear();

        engine::ProcessContext context;
        context.output       = pool.buffer(0);
        context.frameCount   = blockSize;
        context.sampleRate   = 48000.0;
        context.playPosition = at;
        context.playing      = true;

        node.process(context);

        const engine::Sample* out = pool.buffer(0).channel(0);
        return std::vector<engine::Sample>(out, out + blockSize);
    }
};

} // namespace

TEST_CASE("a node with no scheduler plays its clips exactly as it always did")
{
    engine::AudioClipNode node;

    engine::AudioClipNode::PlacedClip clip;
    clip.audio  = flat(1000, 0.5f);
    clip.start  = 100;
    clip.length = 1000;
    node.addClip(std::move(clip));

    node.prepare(48000.0, 512);

    engine::AudioBufferPool pool;
    pool.allocate(1, 2, 512);
    pool.buffer(0).clear();

    engine::ProcessContext context;
    context.output       = pool.buffer(0);
    context.frameCount   = 512;
    context.sampleRate   = 48000.0;
    context.playPosition = 0;
    context.playing      = true;

    node.process(context);

    CHECK(pool.buffer(0).channel(0)[99] == 0.0f);
    CHECK(pool.buffer(0).channel(0)[100] == doctest::Approx(0.5f));
}

TEST_CASE("an untriggered performance track is silent even though its clips are placed")
{
    NodeFixture fixture{track(2, barFrames * 4)};

    const auto block = fixture.render(0);
    for (const engine::Sample sample : block)
        CHECK(sample == 0.0f);
}

TEST_CASE("a triggered clip starts on its exact frame, inside the block that contains it")
{
    // A one-beat grid: the trigger below lands at 24,000, which sits 100
    // frames into a block starting at 23,900.
    NodeFixture fixture{track(2, barFrames * 4, PerformancePress::retrigger,
                              PerformanceMotion::stay, ticksPerQuarterNote)};

    REQUIRE(fixture.scheduler.postTrigger(0, 1, true, quarter - 3000));

    const auto block = fixture.render(quarter - 100, 512);

    // Silence up to the beat, and the clip's own level from it — in one block,
    // which is the whole claim the design makes about quantised triggers.
    for (std::size_t index = 0; index < 100; ++index)
        CHECK(block[index] == 0.0f);

    for (std::size_t index = 100; index < 512; ++index)
        CHECK(block[index] == doctest::Approx(0.75f));
}

TEST_CASE("a clip handing over mid-block does so on the frame, not at the block edge")
{
    // Two clips of 300 frames each, marching. Triggered at zero, the second
    // takes over at frame 300 — a third of the way into the first block.
    PerformanceScheduler::TrackSetup setup;
    setup.motion = PerformanceMotion::marchAndWrap;
    setup.clips  = {{0, 300}, {0, 300}};

    NodeFixture fixture{setup};
    REQUIRE(fixture.scheduler.postTrigger(0, 0, true, 0));

    const auto block = fixture.render(0, 512);

    for (std::size_t index = 0; index < 300; ++index)
        CHECK(block[index] == doctest::Approx(0.25f));

    for (std::size_t index = 300; index < 512; ++index)
        CHECK(block[index] == doctest::Approx(0.75f));
}

TEST_CASE("a released hold falls silent on its frame")
{
    NodeFixture fixture{track(2, barFrames * 4, PerformancePress::hold)};

    REQUIRE(fixture.scheduler.postTrigger(0, 0, true, 0));
    REQUIRE(fixture.scheduler.postTrigger(0, 0, false, 200));

    const auto block = fixture.render(0, 512);

    for (std::size_t index = 0; index < 200; ++index)
        CHECK(block[index] == doctest::Approx(0.25f));

    for (std::size_t index = 200; index < 512; ++index)
        CHECK(block[index] == 0.0f);
}

TEST_CASE("rendering a performance block allocates nothing and takes no lock")
{
    NodeFixture fixture{track(2, quarter, PerformancePress::retrigger,
                              PerformanceMotion::marchAndWrap, ticksPerQuarterNote)};

    REQUIRE(fixture.scheduler.postTrigger(0, 0, true, 0));

    engine::AudioBufferPool pool;
    pool.allocate(1, 2, 512);

    engine::rt::resetViolations();

    {
        const engine::rt::ScopedRealtimeContext section;

        for (FramePosition frame = 0; frame < quarter * 20; frame += 512) {
            pool.buffer(0).clear();

            engine::ProcessContext context;
            context.output       = pool.buffer(0);
            context.frameCount   = 512;
            context.sampleRate   = 48000.0;
            context.playPosition = frame;
            context.playing      = true;

            fixture.node.process(context);
        }
    }

    CHECK(engine::rt::allocationViolations() == 0);
    CHECK(engine::rt::deallocationViolations() == 0);
}

// ── TRACK B (B12, increment 4) — the project side ────────────────────────────
//
// The settings are arrangement data like any other: undoable, saved, shared
// with nothing. The load-bearing test is the last one — a start marker, a mode
// flag and a trigger produce a sound through the whole compiled graph, which
// is the first point at which any of this is reachable from a project.

#include "app/CommandRegistry.h"
#include "app/commands/MarkerCommands.h"
#include "app/commands/PerformanceCommands.h"
#include "engine/audio/WavFile.h"
#include "project/ProjectFile.h"
#include "project/ProjectGraphCompiler.h"

#include <atomic>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

struct ScratchDir {
    explicit ScratchDir(const std::string& name)
        : path(fs::temp_directory_path() / ("incdaw-perf-" + name + "-"
                                            + std::to_string(nextSerial())))
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

    fs::path path;

private:
    static int nextSerial()
    {
        static std::atomic<int> counter{0};
        return ++counter;
    }
};

} // namespace

TEST_CASE("a project has no performance zone until a marker is made the start marker")
{
    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    app::CommandRegistry registry{project};

    CHECK(project::performanceZoneEnd(project) == 0);

    REQUIRE(registry.execute(std::make_unique<app::AddMarkerCommand>(
        ticksPerQuarterNote * 16, std::string{"Song"})));

    const project::EntityId marker = project.markers()[0].id;
    CHECK(project::performanceZoneEnd(project) == 0);   // a marker is not a start marker

    REQUIRE(registry.execute(std::make_unique<app::SetStartMarkerCommand>(marker)));
    CHECK(project::performanceZoneEnd(project) == ticksPerQuarterNote * 16);

    REQUIRE(registry.undo());
    CHECK(project::performanceZoneEnd(project) == 0);
}

TEST_CASE("only one marker is the start marker")
{
    project::Project project;
    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::AddMarkerCommand>(
        ticksPerQuarterNote * 8, std::string{"A"})));
    REQUIRE(registry.execute(std::make_unique<app::AddMarkerCommand>(
        ticksPerQuarterNote * 16, std::string{"B"})));

    const project::EntityId first  = project.markers()[0].id;
    const project::EntityId second = project.markers()[1].id;

    REQUIRE(registry.execute(std::make_unique<app::SetStartMarkerCommand>(first)));
    REQUIRE(registry.execute(std::make_unique<app::SetStartMarkerCommand>(second)));

    CHECK_FALSE(project.findMarker(first)->isStart);
    CHECK(project.findMarker(second)->isStart);
    CHECK(project::performanceZoneEnd(project) == ticksPerQuarterNote * 16);

    // Setting the one that is already the start marker changes nothing.
    CHECK_FALSE(registry.execute(std::make_unique<app::SetStartMarkerCommand>(second)));

    // Undo puts the previous one back rather than leaving none.
    REQUIRE(registry.undo());
    CHECK(project.findMarker(first)->isStart);
    CHECK_FALSE(project.findMarker(second)->isStart);

    // And an invalid id clears it outright.
    REQUIRE(registry.execute(std::make_unique<app::SetStartMarkerCommand>(project::EntityId{})));
    CHECK(project::performanceZoneEnd(project) == 0);
}

TEST_CASE("a track's performance behaviour is one undoable setting")
{
    project::Project project;
    const project::EntityId track =
        project.addTrack(project::TrackType::audio, "Audio").id;

    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::SetTrackPerformanceCommand>(
        track, engine::PerformancePress::latch, engine::PerformanceMotion::marchAndWrap,
        ticksPerQuarterNote * 4, true)));

    const project::Track& after = *project.findTrack(track);
    CHECK(after.performancePress == engine::PerformancePress::latch);
    CHECK(after.performanceMotion == engine::PerformanceMotion::marchAndWrap);
    CHECK(after.triggerSyncTicks == ticksPerQuarterNote * 4);
    CHECK(after.positionSync);

    // Setting what it already is is not an undo entry.
    CHECK_FALSE(registry.execute(std::make_unique<app::SetTrackPerformanceCommand>(
        track, engine::PerformancePress::latch, engine::PerformanceMotion::marchAndWrap,
        ticksPerQuarterNote * 4, true)));

    REQUIRE(registry.undo());
    CHECK(project.findTrack(track)->performancePress == engine::PerformancePress::retrigger);
    CHECK(project.findTrack(track)->triggerSyncTicks == 0);
}

TEST_CASE("a pad answers one clip per track")
{
    project::Project project;

    const project::EntityId track =
        project.addTrack(project::TrackType::audio, "Audio").id;
    const project::EntityId other =
        project.addTrack(project::TrackType::audio, "Other").id;

    auto& asset = project.addAudioAsset("/tmp/incdaw-perf.wav");

    const auto place = [&](project::EntityId on) {
        project::Clip& clip = project.addClip(project::ClipType::audio, on, asset.id);
        clip.length = 1000;
        return clip.id;
    };

    const project::EntityId first  = place(track);
    const project::EntityId second = place(track);
    const project::EntityId across = place(other);

    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::SetClipPerformanceKeyCommand>(first, 7)));
    REQUIRE(registry.execute(std::make_unique<app::SetClipPerformanceKeyCommand>(across, 7)));

    // The same pad on a DIFFERENT track is fine: one pad drives one clip per
    // track, which is what "one clip at a time per track" means.
    CHECK(project.findClip(first)->performanceKey == 7);
    CHECK(project.findClip(across)->performanceKey == 7);

    // On the same track it moves rather than doubling up.
    REQUIRE(registry.execute(std::make_unique<app::SetClipPerformanceKeyCommand>(second, 7)));
    CHECK(project.findClip(second)->performanceKey == 7);
    CHECK(project.findClip(first)->performanceKey == -1);

    REQUIRE(registry.undo());
    CHECK(project.findClip(first)->performanceKey == 7);
    CHECK(project.findClip(second)->performanceKey == -1);
}

TEST_CASE("performance settings round-trip through the project file")
{
    ScratchDir scratch{"roundtrip"};

    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    const project::EntityId track =
        project.addTrack(project::TrackType::audio, "Audio").id;

    auto& asset       = project.addAudioAsset("/tmp/incdaw-perf.wav");
    project::Clip& clip = project.addClip(project::ClipType::audio, track, asset.id);
    clip.length         = 1000;

    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::AddMarkerCommand>(
        ticksPerQuarterNote * 16, std::string{"Song"})));
    REQUIRE(registry.execute(
        std::make_unique<app::SetStartMarkerCommand>(project.markers()[0].id)));
    REQUIRE(registry.execute(std::make_unique<app::SetTrackPerformanceCommand>(
        track, engine::PerformancePress::hold, engine::PerformanceMotion::random,
        ticksPerQuarterNote * 2, true)));
    REQUIRE(registry.execute(std::make_unique<app::SetClipPerformanceKeyCommand>(clip.id, 3)));

    const fs::path file = scratch.path / "Performance.incdaw";
    REQUIRE(bool(project::ProjectFile::save(project, file)));

    project::Project reloaded;
    REQUIRE(bool(project::ProjectFile::load(reloaded, file)));

    CHECK(project::performanceZoneEnd(reloaded) == ticksPerQuarterNote * 16);

    const project::Track& back = *reloaded.findTrack(track);
    CHECK(back.performancePress == engine::PerformancePress::hold);
    CHECK(back.performanceMotion == engine::PerformanceMotion::random);
    CHECK(back.triggerSyncTicks == ticksPerQuarterNote * 2);
    CHECK(back.positionSync);

    CHECK(reloaded.findClip(clip.id)->performanceKey == 3);
    CHECK(reloaded == project);
}

TEST_CASE("a start marker changes nothing until Performance Mode is switched on")
{
    ScratchDir scratch{"compile"};

    // A flat clip inside what will become the zone.
    auto data = flat(4000, 0.5f);
    REQUIRE(bool(engine::WavFile::write(scratch.path / "flat.wav", *data)));

    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    const project::EntityId track =
        project.addTrack(project::TrackType::audio, "Audio").id;

    auto& asset      = project.addAudioAsset((scratch.path / "flat.wav").string());
    asset.sampleRate = 48000.0;
    asset.frameCount = 4000;

    project::Clip& clip = project.addClip(project::ClipType::audio, track, asset.id);
    clip.start          = 0;
    clip.length         = 4000;

    project::TimelineMarker& marker = project.addMarker(ticksPerQuarterNote * 16, "Song");
    marker.isStart                  = true;

    const auto render = [&](bool performanceMode) {
        project::GraphCompileOptions options;
        options.source          = project::PlaybackSource::arrangement;
        options.masterGain      = 1.0f;
        options.performanceMode = performanceMode;

        auto compiled = project::compileProjectGraph(project, project.tempoMap(), options);
        REQUIRE(bool(compiled));

        engine::AudioBufferPool pool;
        pool.allocate(1, 2, 512);
        pool.buffer(0).clear();
        compiled.graph->process(pool.buffer(0), 512, 0);

        struct Rendered {
            engine::Sample                 first;
            engine::PerformanceScheduler*  scheduler;
        };

        return Rendered{pool.buffer(0).channel(0)[10], compiled.performance.get()};
    };

    // Off: the clip plays where it is placed, exactly as it always did.
    CHECK(render(false).first != 0.0f);

    // On: the same clip is in the zone, so it waits to be triggered.
    CHECK(render(true).first == 0.0f);
}

TEST_CASE("a trigger sounds through the whole compiled graph")
{
    ScratchDir scratch{"trigger"};

    auto data = flat(4000, 0.5f);
    REQUIRE(bool(engine::WavFile::write(scratch.path / "flat.wav", *data)));

    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    const project::EntityId track =
        project.addTrack(project::TrackType::audio, "Audio").id;

    auto& asset      = project.addAudioAsset((scratch.path / "flat.wav").string());
    asset.sampleRate = 48000.0;
    asset.frameCount = 4000;

    project::Clip& clip  = project.addClip(project::ClipType::audio, track, asset.id);
    clip.start           = 0;
    clip.length          = 4000;
    clip.performanceKey  = 0;

    project::TimelineMarker& marker = project.addMarker(ticksPerQuarterNote * 16, "Song");
    marker.isStart                  = true;

    project::GraphCompileOptions options;
    options.source          = project::PlaybackSource::arrangement;
    options.masterGain      = 1.0f;
    options.performanceMode = true;

    auto compiled = project::compileProjectGraph(project, project.tempoMap(), options);
    REQUIRE(bool(compiled));

    // The scheduler exists, and knows which playlist track its slot is.
    REQUIRE(compiled.performance != nullptr);
    REQUIRE(compiled.performanceTracks.size() == 1);
    CHECK(compiled.performanceTracks[0] == track);

    engine::AudioBufferPool pool;
    pool.allocate(1, 2, 512);

    // Silent until something asks for it.
    pool.buffer(0).clear();
    compiled.graph->process(pool.buffer(0), 512, 0);
    CHECK(pool.buffer(0).channel(0)[10] == 0.0f);

    REQUIRE(compiled.performance->postTrigger(0, 0, true, 600));

    pool.buffer(0).clear();
    compiled.graph->process(pool.buffer(0), 512, 512);

    // Frame 600 is 88 frames into this block: silence before it, sound from it.
    CHECK(pool.buffer(0).channel(0)[87] == 0.0f);
    CHECK(pool.buffer(0).channel(0)[88] != 0.0f);
}

TEST_CASE("the v1.12 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.12" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    project::Project project;
    const auto result = project::ProjectFile::load(project, fixture);
    REQUIRE(result.succeeded);

    CHECK(project.metadata().title == "Format v1.12 fixture");

    REQUIRE(project.markers().size() == 1);
    CHECK(project.markers()[0].isStart);
    CHECK(project::performanceZoneEnd(project) == ticksPerQuarterNote * 16);

    REQUIRE(project.tracks().size() == 1);
    const project::Track& track = project.tracks()[0];
    CHECK(track.performancePress == engine::PerformancePress::latch);
    CHECK(track.performanceMotion == engine::PerformanceMotion::marchAndWrap);
    CHECK(track.triggerSyncTicks == ticksPerQuarterNote * 4);
    CHECK(track.positionSync);

    REQUIRE(project.clips().size() == 2);
    CHECK(project.clips()[0].performanceKey == 0);
    CHECK(project.clips()[1].performanceKey == 1);
    CHECK(project::clipInPerformanceZone(project, project.clips()[0]));
}

TEST_CASE("a 1.11 project has no performance zone")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.11" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    project::Project project;
    const auto result = project::ProjectFile::load(project, fixture);
    REQUIRE(result.succeeded);
    CHECK(result.migrated);
    CHECK(result.migratedFrom == "1.11");

    CHECK(project::performanceZoneEnd(project) == 0);

    for (const project::Track& track : project.tracks()) {
        CHECK(track.performancePress == engine::PerformancePress::retrigger);
        CHECK(track.triggerSyncTicks == 0);
    }

    for (const project::Clip& clip : project.clips())
        CHECK(clip.performanceKey == -1);
}

// ── TRACK B (B12, increment 5) — the surface's arithmetic ────────────────────
//
// The window is not testable here, but the one piece of it that could quietly
// go wrong is: which clip a pad resolves to. That mapping is recorded by the
// compiler rather than re-derived, and this is the test that says so.

TEST_CASE("a pad resolves to the clip the compiler put behind it")
{
    ScratchDir scratch{"resolve"};

    auto data = flat(4000, 0.5f);
    REQUIRE(bool(engine::WavFile::write(scratch.path / "flat.wav", *data)));

    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    const project::EntityId track =
        project.addTrack(project::TrackType::audio, "Pads").id;

    auto& asset      = project.addAudioAsset((scratch.path / "flat.wav").string());
    asset.sampleRate = 48000.0;
    asset.frameCount = 4000;

    // Deliberately created in the WRONG order and on two lanes, so a naive
    // "clips in project order" mapping would disagree with the compiler's.
    const auto place = [&](engine::FramePosition start, int lane, int pad) {
        project::Clip& clip  = project.addClip(project::ClipType::audio, track, asset.id);
        clip.start           = start;
        clip.length          = 4000;
        clip.lane            = lane;
        clip.performanceKey  = pad;
        return clip.id;   // read before the next add can reallocate the vector
    };

    const project::EntityId lateId  = place(4000, 1, 1);
    const project::EntityId earlyId = place(0, 0, 0);

    project::TimelineMarker& marker = project.addMarker(ticksPerQuarterNote * 16, "Song");
    marker.isStart                  = true;

    project::GraphCompileOptions options;
    options.source          = project::PlaybackSource::arrangement;
    options.masterGain      = 1.0f;
    options.performanceMode = true;

    auto compiled = project::compileProjectGraph(project, project.tempoMap(), options);
    REQUIRE(bool(compiled));
    REQUIRE(compiled.performance != nullptr);

    REQUIRE(compiled.performanceClips.size() == 1);
    REQUIRE(compiled.performanceClips[0].size() == 2);

    // Lane, then start: the clip on lane 0 is first whatever order it was made
    // in, which is exactly what the mapping exists to record.
    CHECK(compiled.performanceClips[0][0] == earlyId);
    CHECK(compiled.performanceClips[0][1] == lateId);

    const auto padZero = project::performanceTargetFor(project, compiled, track, 0);
    REQUIRE(padZero.found);
    CHECK(padZero.slot == 0);
    CHECK(padZero.clip == 0);

    const auto padOne = project::performanceTargetFor(project, compiled, track, 1);
    REQUIRE(padOne.found);
    CHECK(padOne.clip == 1);

    // A pad nothing is bound to, and a track that is not performing.
    CHECK_FALSE(project::performanceTargetFor(project, compiled, track, 5).found);
    CHECK_FALSE(project::performanceTargetFor(project, compiled, project::EntityId{999}, 0).found);
    CHECK_FALSE(project::performanceTargetFor(project, compiled, track, -1).found);
}
