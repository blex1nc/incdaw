#include "doctest.h"

#include "engine/core/RealtimeGuard.h"
#include "engine/midi/MidiBuffer.h"
#include "engine/midi/MidiClock.h"
#include "engine/transport/TempoMap.h"
#include "engine/transport/Transport.h"

#include <cmath>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

constexpr SampleRate rate      = 48000.0;
constexpr FrameCount blockSize = 512;

/// One quarter note at 120 BPM is 24000 frames, so a pulse every 1000.
constexpr double pulseFramesAt120 = 1000.0;

MidiMessage realtime(std::uint8_t status, FrameCount offset)
{
    MidiMessage message;
    message.frameOffset = offset;
    message.status      = status;
    return message;
}

MidiMessage songPosition(int beats, FrameCount offset)
{
    MidiMessage message;
    message.frameOffset = offset;
    message.status      = 0xF2;
    message.data1       = static_cast<std::uint8_t>(beats & 0x7F);
    message.data2       = static_cast<std::uint8_t>((beats >> 7) & 0x7F);
    return message;
}

/// Drives the receiver and the transport over a run of blocks, delivering the
/// messages whose absolute frames fall inside each one.
///
/// Both are driven, in the order the engine drives them, because the thing
/// under test is what the transport ends up doing — not what the receiver
/// intended.
class Session {
public:
    explicit Session(double beatsPerMinute = 120.0)
        : transport(TempoMap{beatsPerMinute, rate})
    {
        receiver.setRole(MidiClockRole::receive);
    }

    void schedule(FrameCount absoluteFrame, const MidiMessage& message)
    {
        pending.emplace_back(absoluteFrame, message);
    }

    /// Runs until `untilFrame`, in blocks.
    void run(FrameCount untilFrame)
    {
        for (; now < untilFrame; now += blockSize) {
            MidiBuffer buffer;

            for (const auto& [frame, message] : pending) {
                if (frame < now || frame >= now + blockSize)
                    continue;

                MidiMessage placed = message;
                placed.frameOffset = frame - now;
                (void)buffer.insert(placed);
            }

            receiver.process(buffer, transport, blockSize, rate);

            BlockSegment plan[Transport::maxSegmentsPerBlock];
            (void)transport.processBlock(blockSize, plan, Transport::maxSegmentsPerBlock);
        }
    }

    /// Adds a run of clock pulses `interval` frames apart.
    void addPulses(FrameCount from, int count, double interval)
    {
        for (int index = 0; index < count; ++index)
            schedule(from + static_cast<FrameCount>(static_cast<double>(index) * interval),
                     realtime(0xF8, 0));
    }

    MidiClockReceiver receiver;
    Transport         transport;

    std::vector<std::pair<FrameCount, MidiMessage>> pending;
    FrameCount                                      now = 0;
};

} // namespace

// ── Off ──────────────────────────────────────────────────────────────────────

TEST_CASE("an unwanted clock does not touch the transport")
{
    Session session;
    session.receiver.setRole(MidiClockRole::off);

    session.schedule(1024, realtime(0xFA, 0));
    session.addPulses(1024, 64, pulseFramesAt120);
    session.run(80000);

    CHECK_FALSE(session.transport.isPlaying());
    CHECK(session.receiver.pulseCount() == 0);
    CHECK(session.receiver.estimatedTempo() == 0.0);
}

// ── The transport follows ────────────────────────────────────────────────────

TEST_CASE("start plays from the beginning wherever the playhead was")
{
    Session session;
    session.transport.seek(session.transport.tempoMap().frameForTick(ticksPerQuarterNote * 16));
    session.run(2048);

    REQUIRE(session.transport.position() > 0);

    session.schedule(4096, realtime(0xFA, 0));
    session.run(8192);

    CHECK(session.transport.isPlaying());

    // Playing since the message, so the position is small but not zero. What
    // matters is that it restarted rather than carrying on from bar five.
    CHECK(session.transport.position() < 8192);
}

TEST_CASE("continue resumes without moving the playhead")
{
    Session session;

    const FramePosition start = session.transport.tempoMap().frameForTick(ticksPerQuarterNote * 8);
    session.transport.seek(start);
    session.run(1024);

    session.schedule(2048, realtime(0xFB, 0));
    session.run(3072);

    CHECK(session.transport.isPlaying());
    CHECK(session.transport.position() >= start);
    CHECK(session.transport.position() < start + 4096);
}

TEST_CASE("stop stops")
{
    Session session;
    session.schedule(1024, realtime(0xFA, 0));
    session.schedule(9000, realtime(0xFC, 0));
    session.run(16000);

    CHECK_FALSE(session.transport.isPlaying());
}

TEST_CASE("a song position pointer locates to the bar it names")
{
    Session session;

    // 32 MIDI beats is 32 sixteenths, which is 8 quarters — two bars of 4/4.
    session.schedule(1024, songPosition(32, 0));
    session.run(4096);

    CHECK(session.receiver.externalTick() == ticksPerQuarterNote * 8);

    // At 120 BPM a quarter is 24000 frames, so 8 of them is 192000. The
    // transport is stopped, so the position is exactly the locate.
    CHECK(session.transport.position() == 192000);
}

// ── The tempo estimate ───────────────────────────────────────────────────────

TEST_CASE("a steady clock is measured, and settles")
{
    Session session;
    session.schedule(0, realtime(0xFA, 0));
    session.addPulses(0, 200, pulseFramesAt120);
    session.run(220000);

    CHECK(session.receiver.isLocked());
    CHECK(session.receiver.estimatedTempo() == doctest::Approx(120.0).epsilon(0.02));
    CHECK(session.receiver.pulseCount() == 200);
    CHECK(session.receiver.rejectedCount() == 0);
}

TEST_CASE("the estimate does not shake with a jittery clock")
{
    Session session;

    // Alternating early and late by a fifth of the interval — far worse than a
    // real bus, and the point is that the reported tempo stays a number a user
    // would believe rather than one that visibly flickers.
    FrameCount frame = 0;
    for (int index = 0; index < 300; ++index) {
        session.schedule(frame, realtime(0xF8, 0));
        const double interval = (index % 2 == 0) ? pulseFramesAt120 * 1.2 : pulseFramesAt120 * 0.8;
        frame += static_cast<FrameCount>(interval);
    }

    session.run(340000);

    CHECK(session.receiver.estimatedTempo() == doctest::Approx(120.0).epsilon(0.06));
}

TEST_CASE("a master that really changes tempo is followed")
{
    Session session;

    // 120 for a while, then 200 — a jump well outside the tolerance, which a
    // filter with no way to give in would reject forever and sit at 120.
    FrameCount frame = 0;
    for (int index = 0; index < 120; ++index) {
        session.schedule(frame, realtime(0xF8, 0));
        frame += static_cast<FrameCount>(pulseFramesAt120);
    }
    for (int index = 0; index < 400; ++index) {
        session.schedule(frame, realtime(0xF8, 0));
        frame += static_cast<FrameCount>(pulseFramesAt120 * 120.0 / 200.0);
    }

    session.run(frame + blockSize * 2);

    CHECK(session.receiver.rejectedCount() > 0);
    CHECK(session.receiver.estimatedTempo() == doctest::Approx(200.0).epsilon(0.03));
}

TEST_CASE("a tempo change inside the tolerance is followed without a stumble")
{
    Session session;

    // 120 to 140 is inside the filter's idea of slop, so nothing is rejected
    // and the estimate simply walks across. A filter that treated every change
    // as an outlier would need a jump before it moved at all.
    FrameCount frame = 0;
    for (int index = 0; index < 120; ++index) {
        session.schedule(frame, realtime(0xF8, 0));
        frame += static_cast<FrameCount>(pulseFramesAt120);
    }
    for (int index = 0; index < 400; ++index) {
        session.schedule(frame, realtime(0xF8, 0));
        frame += static_cast<FrameCount>(pulseFramesAt120 * 120.0 / 140.0);
    }

    session.run(frame + blockSize * 2);

    CHECK(session.receiver.rejectedCount() == 0);
    CHECK(session.receiver.estimatedTempo() == doctest::Approx(140.0).epsilon(0.03));
}

TEST_CASE("a clock that goes away stops being trusted, and does not stop the song")
{
    Session session;
    session.schedule(0, realtime(0xFA, 0));
    session.addPulses(0, 100, pulseFramesAt120);

    session.run(110000);
    REQUIRE(session.receiver.isLocked());

    // More than a second of silence.
    session.run(110000 + static_cast<FrameCount>(rate * 1.5));

    CHECK_FALSE(session.receiver.isLocked());

    // Still playing. A cable knocked out mid-take is not a reason to silence
    // the session; only an explicit stop stops it.
    CHECK(session.transport.isPlaying());
}

// ── Position ─────────────────────────────────────────────────────────────────

TEST_CASE("the reported musical position advances 40 ticks per pulse")
{
    Session session;
    session.schedule(0, realtime(0xFA, 0));
    session.addPulses(1000, 24, pulseFramesAt120);
    session.run(40000);

    // 24 pulses is exactly one quarter note.
    CHECK(session.receiver.externalTick() == ticksPerQuarterNote);
}

TEST_CASE("a locate re-bases the reported position")
{
    Session session;
    session.schedule(0, realtime(0xFA, 0));
    session.addPulses(1000, 48, pulseFramesAt120);
    session.schedule(60000, songPosition(4, 0));   // one quarter in
    session.addPulses(61000, 12, pulseFramesAt120);
    session.run(90000);

    CHECK(session.receiver.externalTick()
          == ticksPerQuarterNote + MidiClockReceiver::ticksPerPulse * 12);
}

// ── Housekeeping ─────────────────────────────────────────────────────────────

TEST_CASE("receiving a block allocates nothing")
{
    MidiClockReceiver receiver;
    receiver.setRole(MidiClockRole::receive);

    Transport  transport{TempoMap{120.0, rate}};
    MidiBuffer buffer;
    (void)buffer.insert(realtime(0xF8, 0));
    (void)buffer.insert(realtime(0xF8, 100));
    (void)buffer.insert(songPosition(16, 200));
    (void)buffer.insert(realtime(0xFB, 300));
    (void)buffer.insert(realtime(0xFC, 400));

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        for (int index = 0; index < 64; ++index)
            receiver.process(buffer, transport, blockSize, rate);
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);

    if (!rt::guardEnabled())
        MESSAGE("realtime guard disabled in this build — allocation not verified");
}

TEST_CASE("reset forgets the master")
{
    Session session;
    session.schedule(0, realtime(0xFA, 0));
    session.addPulses(0, 100, pulseFramesAt120);
    session.run(110000);
    REQUIRE(session.receiver.isLocked());

    session.receiver.reset();

    CHECK_FALSE(session.receiver.isLocked());
    CHECK(session.receiver.estimatedTempo() == 0.0);
    CHECK(session.receiver.pulseCount() == 0);
    CHECK(session.receiver.externalTick() == 0);
}
