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
