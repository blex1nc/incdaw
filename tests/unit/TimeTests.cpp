#include "doctest.h"

#include "engine/core/Time.h"

using namespace incdaw::engine;

TEST_CASE("frames and seconds round-trip at common sample rates")
{
    for (const SampleRate rate : {44100.0, 48000.0, 88200.0, 96000.0, 192000.0}) {
        for (const FrameCount frames : {FrameCount{0}, FrameCount{1}, FrameCount{4410},
                                        FrameCount{48000}, FrameCount{123456789}}) {
            const double     seconds = framesToSeconds(frames, rate);
            const FrameCount back    = secondsToFrames(seconds, rate);
            CHECK(back == frames);
        }
    }
}

TEST_CASE("frame conversion rounds to nearest rather than truncating")
{
    // 0.9 frames must become 1, not 0. Truncation here would introduce a
    // systematic backward drift across many independent conversions.
    CHECK(secondsToFrames(0.9 / 48000.0, 48000.0) == 1);
    CHECK(secondsToFrames(0.4 / 48000.0, 48000.0) == 0);
    CHECK(secondsToFrames(-0.9 / 48000.0, 48000.0) == -1);
}

TEST_CASE("a zero or negative sample rate cannot produce garbage")
{
    CHECK(framesToSeconds(48000, 0.0) == 0.0);
    CHECK(secondsToFrames(1.0, 0.0) == 0);
}

TEST_CASE("ticks and seconds agree with the definition of BPM")
{
    // At 120 BPM a quarter note is exactly half a second.
    CHECK(ticksToSeconds(ticksPerQuarterNote, 120.0) == doctest::Approx(0.5));
    CHECK(ticksToSeconds(ticksPerQuarterNote * 4, 120.0) == doctest::Approx(2.0));
    // At 60 BPM a quarter note is exactly one second.
    CHECK(ticksToSeconds(ticksPerQuarterNote, 60.0) == doctest::Approx(1.0));

    CHECK(secondsToTicks(0.5, 120.0) == ticksPerQuarterNote);
    CHECK(secondsToTicks(1.0, 60.0) == ticksPerQuarterNote);
}

TEST_CASE("the tick resolution divides every common note value exactly")
{
    // This is the reason 960 was chosen; if it is ever changed, this test says
    // what the change would break.
    for (const Tick division : {1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 16, 20, 24, 30, 32, 64, 96})
        CHECK(ticksPerQuarterNote % division == 0);
}

TEST_CASE("time signatures reject non-power-of-two denominators")
{
    CHECK(TimeSignature{4, 4}.isValid());
    CHECK(TimeSignature{7, 8}.isValid());
    CHECK(TimeSignature{3, 16}.isValid());

    CHECK_FALSE(TimeSignature{4, 3}.isValid());   // "3" is not a note value
    CHECK_FALSE(TimeSignature{0, 4}.isValid());
    CHECK_FALSE(TimeSignature{4, 0}.isValid());
    CHECK_FALSE(TimeSignature{-1, 4}.isValid());
}

TEST_CASE("bar and beat lengths follow the time signature")
{
    CHECK(TimeSignature{4, 4}.ticksPerBeat() == ticksPerQuarterNote);
    CHECK(TimeSignature{4, 4}.ticksPerBar()  == ticksPerQuarterNote * 4);

    // In 6/8 the beat is an eighth note: half a quarter.
    CHECK(TimeSignature{6, 8}.ticksPerBeat() == ticksPerQuarterNote / 2);
    CHECK(TimeSignature{6, 8}.ticksPerBar()  == ticksPerQuarterNote * 3);
}

TEST_CASE("musical positions round-trip through ticks")
{
    const TimeSignature signatures[] = {{4, 4}, {3, 4}, {7, 8}, {5, 4}, {12, 8}};

    for (const auto signature : signatures) {
        for (Tick ticks = 0; ticks < signature.ticksPerBar() * 3; ticks += 37) {
            const auto position = ticksToMusicalPosition(ticks, signature);
            CHECK(musicalPositionToTicks(position, signature) == ticks);

            CHECK(position.bar >= 1);
            CHECK(position.beat >= 1);
            CHECK(position.beat <= signature.numerator);
            CHECK(position.tick >= 0);
            CHECK(position.tick < signature.ticksPerBeat());
        }
    }
}

TEST_CASE("bars and beats are 1-based, as shown on a ruler")
{
    const TimeSignature fourFour{4, 4};

    CHECK(ticksToMusicalPosition(0, fourFour) == MusicalPosition{1, 1, 0});
    CHECK(ticksToMusicalPosition(ticksPerQuarterNote, fourFour) == MusicalPosition{1, 2, 0});
    CHECK(ticksToMusicalPosition(ticksPerQuarterNote * 4, fourFour) == MusicalPosition{2, 1, 0});
    CHECK(ticksToMusicalPosition(ticksPerQuarterNote * 4 + 480, fourFour) == MusicalPosition{2, 1, 480});
}

TEST_CASE("negative positions walk backwards instead of collapsing onto bar 1")
{
    // Count-in and pre-roll live before the song start. Truncating division
    // would map every negative tick into bar 0 or bar 1 and break the ruler.
    const TimeSignature fourFour{4, 4};
    const Tick          oneBar = fourFour.ticksPerBar();

    CHECK(ticksToMusicalPosition(-oneBar, fourFour) == MusicalPosition{0, 1, 0});
    CHECK(ticksToMusicalPosition(-oneBar * 2, fourFour) == MusicalPosition{-1, 1, 0});
    CHECK(ticksToMusicalPosition(-ticksPerQuarterNote, fourFour) == MusicalPosition{0, 4, 0});

    CHECK(musicalPositionToTicks(ticksToMusicalPosition(-oneBar - 13, fourFour), fourFour)
          == -oneBar - 13);
}

TEST_CASE("an invalid time signature yields a defined result rather than a division by zero")
{
    const TimeSignature broken{4, 3};
    CHECK(ticksToMusicalPosition(1000, broken) == MusicalPosition{1, 1, 0});
    CHECK(musicalPositionToTicks({2, 2, 10}, broken) == 0);
}
