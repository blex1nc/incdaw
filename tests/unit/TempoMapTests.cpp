#include "doctest.h"

#include "engine/core/RealtimeGuard.h"
#include "engine/transport/TempoMap.h"

using namespace incdaw::engine;

TEST_CASE("a constant tempo map converts ticks and frames consistently")
{
    const TempoMap map{120.0, 48000.0};

    // At 120 BPM a quarter note is 0.5 s = 24000 frames at 48 kHz.
    CHECK(map.frameForTick(ticksPerQuarterNote) == 24000);
    CHECK(map.frameForTick(ticksPerQuarterNote * 4) == 96000);
    CHECK(map.tickForFrame(24000) == ticksPerQuarterNote);
    CHECK(map.tickForFrame(0) == 0);
}

TEST_CASE("tick and frame conversion round-trips")
{
    const TempoMap map{140.0, 44100.0};

    for (Tick tick = 0; tick < ticksPerQuarterNote * 64; tick += 137) {
        const FramePosition frame = map.frameForTick(tick);
        CHECK(map.tickForFrame(frame) == tick);
    }
}

TEST_CASE("the map is total: a query before the first event still answers")
{
    TempoMap map;
    map.setTempoEvents({{ticksPerQuarterNote * 8, 90.0}});   // no event at 0

    CHECK(map.tempoAtTick(0) == doctest::Approx(90.0));
    CHECK(map.frameForTick(0) == 0);
}

TEST_CASE("tempo changes take effect at their own tick, not before")
{
    TempoMap map{120.0, 48000.0};
    map.setTempoEvents({{0, 120.0}, {ticksPerQuarterNote * 4, 240.0}});

    CHECK(map.tempoAtTick(0) == doctest::Approx(120.0));
    CHECK(map.tempoAtTick(ticksPerQuarterNote * 4 - 1) == doctest::Approx(120.0));
    CHECK(map.tempoAtTick(ticksPerQuarterNote * 4) == doctest::Approx(240.0));

    // One bar at 120 BPM is 2 s = 96000 frames.
    CHECK(map.frameForTick(ticksPerQuarterNote * 4) == 96000);

    // The next bar runs at 240 BPM, so it takes half as long: 48000 frames.
    CHECK(map.frameForTick(ticksPerQuarterNote * 8) == 96000 + 48000);
}

TEST_CASE("conversion round-trips across a tempo change")
{
    TempoMap map{120.0, 48000.0};
    map.setTempoEvents({{0, 120.0}, {ticksPerQuarterNote * 4, 75.0}, {ticksPerQuarterNote * 9, 180.0}});

    for (Tick tick = 0; tick < ticksPerQuarterNote * 20; tick += 61) {
        const FramePosition frame = map.frameForTick(tick);
        CHECK(map.tickForFrame(frame) == tick);
    }
}

TEST_CASE("tempo events are sorted and de-duplicated regardless of input order")
{
    TempoMap map;
    map.setTempoEvents({{ticksPerQuarterNote * 8, 100.0},
                        {0, 120.0},
                        {ticksPerQuarterNote * 8, 150.0},   // same tick, later wins
                        {ticksPerQuarterNote * 4, 90.0}});

    REQUIRE(map.tempoEvents().size() == 3);
    CHECK(map.tempoEvents()[0].tick == 0);
    CHECK(map.tempoEvents()[1].tick == ticksPerQuarterNote * 4);
    CHECK(map.tempoEvents()[2].tick == ticksPerQuarterNote * 8);
}

TEST_CASE("nonsensical tempi are clamped instead of dividing by zero downstream")
{
    TempoMap map;
    map.setTempoEvents({{0, 0.0}, {ticksPerQuarterNote, -50.0}, {ticksPerQuarterNote * 2, 100000.0}});

    CHECK(map.tempoAtTick(0) > 0.0);
    CHECK(map.tempoAtTick(ticksPerQuarterNote) > 0.0);
    CHECK(map.tempoAtTick(ticksPerQuarterNote * 2) <= 999.0);

    // Every position must still map to a finite, non-decreasing frame.
    CHECK(map.frameForTick(ticksPerQuarterNote * 4) > 0);
}

TEST_CASE("events at negative positions are dropped")
{
    TempoMap map;
    map.setTempoEvents({{-1000, 200.0}, {0, 120.0}});

    for (const auto& event : map.tempoEvents())
        CHECK(event.tick >= 0);
}

TEST_CASE("changing the sample rate rescales every frame position")
{
    TempoMap map{120.0, 48000.0};
    CHECK(map.frameForTick(ticksPerQuarterNote) == 24000);

    map.setSampleRate(96000.0);
    CHECK(map.frameForTick(ticksPerQuarterNote) == 48000);

    map.setSampleRate(44100.0);
    CHECK(map.frameForTick(ticksPerQuarterNote) == 22050);
}

TEST_CASE("time signature lookup honours changes")
{
    TempoMap map;
    map.setTimeSignatureEvents({{0, {4, 4}}, {ticksPerQuarterNote * 8, {7, 8}}});

    CHECK(map.timeSignatureAtTick(0) == TimeSignature{4, 4});
    CHECK(map.timeSignatureAtTick(ticksPerQuarterNote * 8 - 1) == TimeSignature{4, 4});
    CHECK(map.timeSignatureAtTick(ticksPerQuarterNote * 8) == TimeSignature{7, 8});
}

TEST_CASE("invalid time signatures are rejected")
{
    TempoMap map;
    map.setTimeSignatureEvents({{0, {4, 4}}, {ticksPerQuarterNote * 4, {5, 3}}});

    for (const auto& event : map.timeSignatureEvents())
        CHECK(event.signature.isValid());
}

TEST_CASE("bar numbering stays correct across a signature change")
{
    // Two bars of 4/4, then 7/8. If bar numbering simply divided by a fixed bar
    // length, the ruler would drift from the third bar onwards.
    TempoMap map;
    map.setTimeSignatureEvents({{0, {4, 4}}, {ticksPerQuarterNote * 8, {7, 8}}});

    CHECK(map.musicalPositionForTick(0) == MusicalPosition{1, 1, 0});
    CHECK(map.musicalPositionForTick(ticksPerQuarterNote * 4) == MusicalPosition{2, 1, 0});
    CHECK(map.musicalPositionForTick(ticksPerQuarterNote * 8) == MusicalPosition{3, 1, 0});

    // A 7/8 bar is 7 eighth notes = 3.5 quarter notes.
    const Tick sevenEighthsBar = ticksPerQuarterNote * 7 / 2;
    CHECK(map.musicalPositionForTick(ticksPerQuarterNote * 8 + sevenEighthsBar)
          == MusicalPosition{4, 1, 0});
}

TEST_CASE("the next tempo change can be located from a frame position")
{
    TempoMap map{120.0, 48000.0};
    map.setTempoEvents({{0, 120.0}, {ticksPerQuarterNote * 4, 60.0}});

    CHECK(map.nextTempoChangeAfter(0) == 96000);
    CHECK(map.nextTempoChangeAfter(95999) == 96000);
    CHECK(map.nextTempoChangeAfter(96000) == TempoMap::noTempoChange);
}

TEST_CASE("map queries are realtime-safe")
{
    TempoMap map{120.0, 48000.0};
    map.setTempoEvents({{0, 120.0}, {ticksPerQuarterNote * 4, 90.0}, {ticksPerQuarterNote * 16, 150.0}});

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;

        volatile FramePosition sink = 0;
        for (Tick tick = 0; tick < ticksPerQuarterNote * 32; tick += 7) {
            sink = map.frameForTick(tick);
            sink = map.tickForFrame(static_cast<FramePosition>(tick) * 10);
            sink = static_cast<FramePosition>(map.tempoAtTick(tick));
        }
        (void)sink;
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
}
