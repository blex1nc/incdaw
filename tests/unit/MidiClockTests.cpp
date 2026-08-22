#include "doctest.h"

#include "engine/core/RealtimeGuard.h"
#include "engine/midi/MidiBuffer.h"
#include "app/AppSettings.h"
#include "engine/midi/MidiClock.h"
#include "engine/transport/TempoMap.h"
#include "engine/transport/Transport.h"

#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

constexpr SampleRate rate      = 48000.0;
constexpr FrameCount blockSize = 512;

/// Every message one run of blocks produced, with the absolute frame each
/// landed on. Absolute rather than per-block, because what is being tested is
/// spacing across block boundaries — the thing a block-quantised clock gets
/// wrong.
struct Emitted {
    std::vector<std::pair<FrameCount, MidiMessage>> messages;

    [[nodiscard]] std::vector<FrameCount> framesOf(std::uint8_t status) const
    {
        std::vector<FrameCount> frames;
        for (const auto& [frame, message] : messages)
            if (message.status == status)
                frames.push_back(frame);
        return frames;
    }

    [[nodiscard]] std::vector<std::uint8_t> statuses() const
    {
        std::vector<std::uint8_t> result;
        for (const auto& [frame, message] : messages)
            result.push_back(message.status);
        return result;
    }

    [[nodiscard]] std::size_t countOf(std::uint8_t status) const { return framesOf(status).size(); }
};

/// Runs `blocks` blocks of the transport through the generator.
Emitted run(MidiClockGenerator& clock, Transport& transport, int blocks,
            FrameCount size = blockSize)
{
    Emitted emitted;
    MidiBuffer buffer;

    for (int index = 0; index < blocks; ++index) {
        BlockSegment plan[Transport::maxSegmentsPerBlock];
        const std::size_t count = transport.processBlock(size, plan, Transport::maxSegmentsPerBlock);
        const bool playing = transport.isPlaying();

        buffer.clear();
        clock.generate(buffer, plan, count, transport.tempoMap(), playing, size);

        for (const MidiMessage& message : buffer)
            emitted.messages.emplace_back(static_cast<FrameCount>(index) * size + message.frameOffset,
                                          message);
    }

    return emitted;
}

/// Gaps between consecutive values.
std::vector<FrameCount> gaps(const std::vector<FrameCount>& frames)
{
    std::vector<FrameCount> result;
    for (std::size_t index = 1; index < frames.size(); ++index)
        result.push_back(frames[index] - frames[index - 1]);
    return result;
}

} // namespace

// ── Off ──────────────────────────────────────────────────────────────────────

TEST_CASE("the clock is silent until it is asked for")
{
    MidiClockGenerator clock;
    Transport          transport{TempoMap{120.0, rate}};
    transport.play();

    CHECK(run(clock, transport, 16).messages.empty());
    CHECK(clock.pulseCount() == 0);
}

// ── Spacing ──────────────────────────────────────────────────────────────────

TEST_CASE("pulses fall 24 to the quarter note")
{
    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport transport{TempoMap{120.0, rate}};
    transport.play();

    // One second at 120 BPM is two quarter notes: 48 pulses. 48000 frames is
    // 93.75 blocks of 512, so the last partial block is left out of the count
    // rather than fudged.
    const Emitted emitted = run(clock, transport, 93);

    const std::vector<FrameCount> pulses = emitted.framesOf(0xF8);
    REQUIRE(pulses.size() >= 47);

    // A quarter note is 24000 frames; a pulse every 1000.
    for (const FrameCount gap : gaps(pulses))
        CHECK(gap == 1000);

    // The first pulse is on the first frame of the song, not a block later.
    CHECK(pulses.front() == 0);
}

TEST_CASE("pulse spacing follows a tempo change written on the timeline")
{
    TempoMap map{120.0, rate};
    map.setTempoEvents({{0, 120.0}, {ticksPerQuarterNote * 4, 60.0}});

    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport transport{map};
    transport.play();

    // Four quarters at 120 (96000 frames), then into the half-speed section.
    const Emitted emitted = run(clock, transport, 300);
    const std::vector<FrameCount> pulses = emitted.framesOf(0xF8);
    REQUIRE(pulses.size() > 100);

    const std::vector<FrameCount> spacing = gaps(pulses);

    // Before the change: 1000 frames. After: 2000. A generator that averaged
    // the tempo, or read it once at the start, would report one number for the
    // whole run — which is the failure this test exists to name.
    CHECK(spacing.front() == 1000);
    CHECK(spacing.back() == 2000);

    bool sawBoth = false;
    for (std::size_t index = 1; index < spacing.size(); ++index)
        if (spacing[index - 1] == 1000 && spacing[index] == 2000)
            sawBoth = true;

    CHECK(sawBoth);
}

// ── Transport transitions ────────────────────────────────────────────────────

TEST_CASE("playing from the top sends start, not a position")
{
    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport transport{TempoMap{120.0, rate}};

    (void)run(clock, transport, 2);   // stopped: free-running clock only
    transport.play();

    const Emitted emitted = run(clock, transport, 1);

    CHECK(emitted.countOf(0xFA) == 1);
    CHECK(emitted.countOf(0xF2) == 0);
    CHECK(emitted.countOf(0xFB) == 0);
    CHECK(emitted.framesOf(0xFA).front() == 0);
}

TEST_CASE("playing from elsewhere sends stop, position and continue in that order")
{
    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport transport{TempoMap{120.0, rate}};

    // Two bars in: 8 quarters = 7680 ticks, which is 32 song-position beats.
    transport.seek(transport.tempoMap().frameForTick(ticksPerQuarterNote * 8));
    (void)run(clock, transport, 1);
    transport.play();

    const Emitted emitted = run(clock, transport, 1);
    const std::vector<std::uint8_t> statuses = emitted.statuses();

    REQUIRE(statuses.size() >= 3);
    CHECK(statuses[0] == 0xFC);
    CHECK(statuses[1] == 0xF2);
    CHECK(statuses[2] == 0xFB);

    // 8 quarters is 32 sixteenths, and the pointer is 14 bits little-end first.
    const MidiMessage& pointer = emitted.messages[1].second;
    const int          beats   = pointer.data1 | (pointer.data2 << 7);
    CHECK(beats == 32);
}

TEST_CASE("stopping sends stop")
{
    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport transport{TempoMap{120.0, rate}};
    transport.play();
    (void)run(clock, transport, 4);

    transport.stop();
    const Emitted emitted = run(clock, transport, 1);

    CHECK(emitted.countOf(0xFC) == 1);
}

TEST_CASE("a loop wrap tells the receiver where the song went")
{
    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport transport{TempoMap{120.0, rate}};
    transport.setLoopRange(0, 4800);   // a tenth of a second, so a wrap happens quickly
    transport.setLoopEnabled(true);
    transport.play();

    const Emitted emitted = run(clock, transport, 40);

    // Without this the gear plays straight on past the loop point while INCDAW
    // is back at the top, and the two drift apart by exactly the loop length.
    CHECK(emitted.countOf(0xF2) >= 3);
    CHECK(emitted.countOf(0xFB) == emitted.countOf(0xF2));
}

TEST_CASE("a seek during playback is bracketed too")
{
    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport transport{TempoMap{120.0, rate}};
    transport.play();
    (void)run(clock, transport, 4);

    transport.seek(transport.tempoMap().frameForTick(ticksPerQuarterNote * 16));
    const Emitted emitted = run(clock, transport, 2);

    REQUIRE(emitted.countOf(0xF2) == 1);
    const MidiMessage& pointer = emitted.messages[static_cast<std::size_t>(1)].second;
    CHECK((pointer.data1 | (pointer.data2 << 7)) == 64);   // 16 quarters = 64 sixteenths
}

// ── Stopped ──────────────────────────────────────────────────────────────────

TEST_CASE("the clock free-runs while the transport is stopped")
{
    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport transport{TempoMap{120.0, rate}};

    // A second of stopped time is still 48 pulses at 120 BPM. Gear with an
    // arpeggiator needs the tempo before the transport moves, not after.
    const Emitted emitted = run(clock, transport, 94);
    const std::size_t pulses = emitted.countOf(0xF8);

    CHECK(pulses >= 46);
    CHECK(pulses <= 50);

    CHECK(emitted.countOf(0xFA) == 0);
    CHECK(emitted.countOf(0xFC) == 0);
}

TEST_CASE("the free-running rate follows the stopped tempo")
{
    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport fast{TempoMap{240.0, rate}};
    const std::size_t atFast = run(clock, fast, 94).countOf(0xF8);

    MidiClockGenerator slowClock;
    slowClock.setRole(MidiClockRole::send);
    Transport slow{TempoMap{60.0, rate}};
    const std::size_t atSlow = run(slowClock, slow, 94).countOf(0xF8);

    CHECK(atFast > atSlow * 3);   // four times the tempo, minus block granularity
}

// ── Housekeeping ─────────────────────────────────────────────────────────────

TEST_CASE("reset forgets the previous run")
{
    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport transport{TempoMap{120.0, rate}};
    transport.play();
    (void)run(clock, transport, 8);
    CHECK(clock.pulseCount() > 0);

    clock.reset();
    CHECK(clock.pulseCount() == 0);

    // The transport is still playing, but the clock has no history: it treats
    // this as a fresh start rather than continuing a phase from a device that
    // is no longer running.
    const Emitted emitted = run(clock, transport, 1);
    CHECK(emitted.countOf(0xF2) + emitted.countOf(0xFA) == 1);
}

TEST_CASE("generating a block's clock allocates nothing")
{
    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport transport{TempoMap{120.0, rate}};
    transport.play();

    MidiBuffer buffer;
    BlockSegment plan[Transport::maxSegmentsPerBlock];

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;

        for (int index = 0; index < 64; ++index) {
            const std::size_t count =
                transport.processBlock(blockSize, plan, Transport::maxSegmentsPerBlock);
            buffer.clear();
            clock.generate(buffer, plan, count, transport.tempoMap(), true, blockSize);
        }
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);

    if (!rt::guardEnabled())
        MESSAGE("realtime guard disabled in this build — allocation not verified");
}

TEST_CASE("an absurd tempo cannot spin the audio thread")
{
    // A corrupt map is not a reason to hang inside a callback. The pulse loop
    // is capped, so the block ends whatever the tempo claims to be.
    TempoMap map{120.0, rate};
    map.setTempoEvents({{0, 100000.0}});

    MidiClockGenerator clock;
    clock.setRole(MidiClockRole::send);

    Transport transport{map};
    transport.play();

    MidiBuffer   buffer;
    BlockSegment plan[Transport::maxSegmentsPerBlock];
    const std::size_t count = transport.processBlock(blockSize, plan, Transport::maxSegmentsPerBlock);

    clock.generate(buffer, plan, count, transport.tempoMap(), true, blockSize);

    CHECK(clock.pulseCount() <= static_cast<std::uint64_t>(MidiClockGenerator::maxPulsesPerBlock)
                                   * Transport::maxSegmentsPerBlock);
}

// ── The stored preference ────────────────────────────────────────────────────

TEST_CASE("the clock role round-trips, and an unknown one reads as off")
{
    app::AppSettings settings;
    CHECK(settings.midiClockRole == "off");

    settings.midiClockRole = "send";
    CHECK(app::AppSettings::fromJson(settings.toJson()).midiClockRole == "send");

    // A value from a build that knows more than this one drives nothing.
    settings.midiClockRole = "receive";
    CHECK(app::AppSettings::fromJson(settings.toJson()).midiClockRole == "receive");

    CHECK(app::AppSettings::fromJson(R"({"midi":{"clockRole":"chase"}})").midiClockRole == "off");
    CHECK(app::AppSettings::fromJson(R"({"midi":{}})").midiClockRole == "off");
}
