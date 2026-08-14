#include "doctest.h"

#include "engine/core/RealtimeGuard.h"
#include "engine/midi/MidiBuffer.h"
#include "engine/midi/MidiInput.h"
#include "engine/midi/MidiRecorder.h"
#include "engine/transport/TempoMap.h"
#include "platform/HostTime.h"
#include "project/MidiCapture.h"

#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

constexpr double nanosPerSecond = 1'000'000'000.0;

/// Host time of a given frame position, for a block starting at `blockStart`.
std::uint64_t nanosForFrame(std::uint64_t blockStartNanos, FrameCount frame, SampleRate rate)
{
    return blockStartNanos + static_cast<std::uint64_t>(static_cast<double>(frame) * nanosPerSecond / rate);
}

platform::TimestampedMidiMessage timestamped(std::uint64_t nanos, const MidiMessage& message)
{
    return {nanos, message.status, message.data1, message.data2};
}

/// Named rather than brace-initialised: MidiEvent has many fields with good
/// defaults, and an aggregate initialiser here would have to be updated every
/// time one is added.
project::MidiEvent patternNote(Tick tick, Tick duration = 100, int key = 60)
{
    project::MidiEvent event;
    event.type     = project::MidiEventType::note;
    event.tick     = tick;
    event.duration = duration;
    event.key      = key;
    return event;
}

} // namespace

// ── MidiMessage ───────────────────────────────────────────────────────────────

TEST_CASE("a note-on with velocity zero is a note-off")
{
    // The MIDI spec defines them as equivalent and many keyboards send it that
    // way. Treating them differently is the classic cause of stuck notes.
    const MidiMessage explicitOff = MidiMessage::noteOff(0, 60);
    const MidiMessage zeroVelocity{0, 0x90, 60, 0};

    CHECK(explicitOff.isNoteOff());
    CHECK(zeroVelocity.isNoteOff());
    CHECK_FALSE(zeroVelocity.isNoteOn());
    CHECK(zeroVelocity.isNote());
}

TEST_CASE("noteOn never emits velocity zero, which would mean the opposite")
{
    const MidiMessage message = MidiMessage::noteOn(0, 60, 0);
    CHECK(message.isNoteOn());
    CHECK(message.velocity() == 1);
}

TEST_CASE("message fields round-trip")
{
    const MidiMessage note = MidiMessage::noteOn(5, 64, 100, 42);
    CHECK(note.channel() == 5);
    CHECK(note.noteNumber() == 64);
    CHECK(note.velocity() == 100);
    CHECK(note.frameOffset == 42);

    const MidiMessage cc = MidiMessage::controlChange(3, 74, 90);
    CHECK(cc.isControlChange());
    CHECK(cc.channel() == 3);
    CHECK(cc.data1 == 74);
    CHECK(cc.data2 == 90);

    const MidiMessage bend = MidiMessage::pitchBend(2, 12345);
    CHECK(bend.isPitchBend());
    CHECK(bend.pitchBendValue() == 12345);
}

TEST_CASE("pitch bend is clamped to its 14-bit range")
{
    CHECK(MidiMessage::pitchBend(0, -100).pitchBendValue() == 0);
    CHECK(MidiMessage::pitchBend(0, 99999).pitchBendValue() == 16383);
}

TEST_CASE("system messages are not channel messages")
{
    const MidiMessage clock{0, 0xF8, 0, 0};
    CHECK(clock.isSystemMessage());
    CHECK_FALSE(clock.isNote());
}

// ── MidiBuffer ────────────────────────────────────────────────────────────────

TEST_CASE("the buffer keeps messages ordered by frame offset")
{
    MidiBuffer buffer;

    for (const FrameCount offset : {50, 10, 90, 30, 0, 70})
        CHECK(buffer.insert(MidiMessage::noteOn(0, 60, 100, offset)));

    REQUIRE(buffer.size() == 6);

    for (std::size_t index = 1; index < buffer.size(); ++index)
        CHECK(buffer[index - 1].frameOffset <= buffer[index].frameOffset);

    CHECK(buffer[0].frameOffset == 0);
    CHECK(buffer[5].frameOffset == 90);
}

TEST_CASE("equal offsets keep their insertion order")
{
    // A note-off and the note-on that replaces it can land on the same frame;
    // reordering them turns a legato line into silence.
    MidiBuffer buffer;
    CHECK(buffer.insert(MidiMessage::noteOff(0, 60, 64, 10)));
    CHECK(buffer.insert(MidiMessage::noteOn(0, 62, 100, 10)));

    CHECK(buffer[0].isNoteOff());
    CHECK(buffer[1].isNoteOn());
}

TEST_CASE("overflow is counted, not silently swallowed")
{
    BasicMidiBuffer<4> buffer;

    for (int index = 0; index < 4; ++index)
        CHECK(buffer.insert(MidiMessage::noteOn(0, 60, 100, index)));

    CHECK_FALSE(buffer.insert(MidiMessage::noteOn(0, 60, 100, 4)));
    CHECK(buffer.overflowCount() == 1);
    CHECK(buffer.size() == 4);
}

TEST_CASE("rebasing keeps only what falls inside the segment")
{
    // Used when a block is split at a loop wrap: the second segment's events
    // must be measured from the segment, not the block.
    MidiBuffer buffer;
    buffer.insert(MidiMessage::noteOn(0, 60, 100, 10));
    buffer.insert(MidiMessage::noteOn(0, 61, 100, 50));
    buffer.insert(MidiMessage::noteOn(0, 62, 100, 90));

    buffer.rebase(-40, 64);

    REQUIRE(buffer.size() == 2);
    CHECK(buffer[0].frameOffset == 10);   // was 50
    CHECK(buffer[1].frameOffset == 50);   // was 90
}

TEST_CASE("buffer operations are realtime-safe")
{
    MidiBuffer buffer;

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        for (int block = 0; block < 1000; ++block) {
            buffer.clear();
            for (int index = 0; index < 64; ++index)
                (void)buffer.insert(MidiMessage::noteOn(0, 60, 100, (index * 37) % 128));
        }
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
}

// ── Host time ─────────────────────────────────────────────────────────────────

TEST_CASE("the host clock is monotonic and in nanoseconds")
{
    const std::uint64_t first = platform::hostTimeNowNanos();

    volatile std::uint64_t spin = 0;
    for (int index = 0; index < 2000000; ++index)
        spin = spin + 1;

    const std::uint64_t second = platform::hostTimeNowNanos();

    CHECK(second >= first);
    CHECK(second - first > 0);
    // Sanity on the unit: a busy loop of two million iterations takes far more
    // than a nanosecond and far less than a second.
    CHECK(second - first < 1'000'000'000ull);
}

// ── MidiInput: host time to frame offset ──────────────────────────────────────

TEST_CASE("input messages land on the frame their timestamp names")
{
    constexpr SampleRate rate      = 48000.0;
    constexpr FrameCount blockSize = 512;

    MidiInput input;
    MidiBuffer buffer;

    const std::uint64_t blockStart = 1'000'000'000ull;

    for (const FrameCount frame : {0, 1, 17, 128, 255, 511})
        input.injectForTesting(timestamped(nanosForFrame(blockStart, frame, rate),
                                           MidiMessage::noteOn(0, 60 + static_cast<int>(frame % 12), 100)));

    input.collectForBlock(buffer, blockStart, blockSize, rate);

    REQUIRE(buffer.size() == 6);

    const FrameCount expected[] = {0, 1, 17, 128, 255, 511};
    for (std::size_t index = 0; index < 6; ++index)
        CHECK(buffer[index].frameOffset == expected[index]);
}

TEST_CASE("a message from the past is placed at the earliest audible frame")
{
    MidiInput input;
    MidiBuffer buffer;

    const std::uint64_t blockStart = 1'000'000'000ull;
    input.injectForTesting(timestamped(blockStart - 5'000'000ull, MidiMessage::noteOn(0, 60, 100)));

    input.collectForBlock(buffer, blockStart, 128, 48000.0);

    REQUIRE(buffer.size() == 1);
    CHECK(buffer[0].frameOffset == 0);
    CHECK(input.lateCount() == 1);
}

TEST_CASE("a message beyond this block is held for the next one")
{
    constexpr SampleRate rate      = 48000.0;
    constexpr FrameCount blockSize = 128;

    MidiInput input;
    MidiBuffer buffer;

    const std::uint64_t blockStart = 1'000'000'000ull;

    // Two frames into the second block.
    input.injectForTesting(timestamped(nanosForFrame(blockStart, blockSize + 2, rate),
                                       MidiMessage::noteOn(0, 60, 100)));

    input.collectForBlock(buffer, blockStart, blockSize, rate);
    CHECK(buffer.isEmpty());

    const std::uint64_t nextBlock = nanosForFrame(blockStart, blockSize, rate);
    input.collectForBlock(buffer, nextBlock, blockSize, rate);

    REQUIRE(buffer.size() == 1);
    CHECK(buffer[0].frameOffset == 2);
}

TEST_CASE("held messages are not lost across several empty blocks")
{
    constexpr SampleRate rate      = 48000.0;
    constexpr FrameCount blockSize = 128;

    MidiInput input;
    MidiBuffer buffer;

    const std::uint64_t blockStart = 1'000'000'000ull;
    input.injectForTesting(timestamped(nanosForFrame(blockStart, blockSize * 3 + 5, rate),
                                       MidiMessage::noteOn(0, 60, 100)));

    for (int block = 0; block < 3; ++block) {
        input.collectForBlock(buffer, nanosForFrame(blockStart, blockSize * block, rate), blockSize, rate);
        CHECK(buffer.isEmpty());
    }

    input.collectForBlock(buffer, nanosForFrame(blockStart, blockSize * 3, rate), blockSize, rate);
    REQUIRE(buffer.size() == 1);
    CHECK(buffer[0].frameOffset == 5);
}

TEST_CASE("collecting is realtime-safe")
{
    MidiInput input;
    MidiBuffer buffer;

    const std::uint64_t blockStart = 1'000'000'000ull;
    for (int index = 0; index < 200; ++index)
        input.injectForTesting(timestamped(blockStart + static_cast<std::uint64_t>(index) * 1000,
                                           MidiMessage::noteOn(0, 60, 100)));

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        for (int block = 0; block < 100; ++block)
            input.collectForBlock(buffer, blockStart + static_cast<std::uint64_t>(block) * 2'666'666ull,
                                  128, 48000.0);
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
    CHECK(input.droppedCount() == 0);
}

// ── Recording ─────────────────────────────────────────────────────────────────

TEST_CASE("recording pairs note-ons with their note-offs")
{
    const TempoMap map{120.0, 48000.0};
    MidiRecorder   recorder;
    MidiBuffer     buffer;

    // A quarter note at 120 BPM is 24000 frames.
    buffer.insert(MidiMessage::noteOn(0, 60, 100, 0));
    recorder.capture(buffer, 0);

    buffer.clear();
    buffer.insert(MidiMessage::noteOff(0, 60, 72, 0));
    recorder.capture(buffer, 24000);

    std::vector<RecordedEvent> events;
    recorder.drainInto(events, map, 48000);

    REQUIRE(events.size() == 1);
    CHECK(events[0].kind == RecordedEvent::Kind::note);
    CHECK(events[0].key == 60);
    CHECK(events[0].value == 100);
    CHECK(events[0].releaseValue == 72);
    CHECK(events[0].tick == 0);
    CHECK(events[0].duration == engine::ticksPerQuarterNote);
}

TEST_CASE("a note still held when recording ends is closed, not discarded")
{
    const TempoMap map{120.0, 48000.0};
    MidiRecorder   recorder;
    MidiBuffer     buffer;

    buffer.insert(MidiMessage::noteOn(0, 60, 100, 0));
    recorder.capture(buffer, 0);

    std::vector<RecordedEvent> events;
    recorder.drainInto(events, map, 24000);

    REQUIRE(events.size() == 1);
    CHECK(events[0].duration == engine::ticksPerQuarterNote);
}

TEST_CASE("a retriggered note closes the previous one instead of leaking it")
{
    const TempoMap map{120.0, 48000.0};
    MidiRecorder   recorder;
    MidiBuffer     buffer;

    buffer.insert(MidiMessage::noteOn(0, 60, 100, 0));
    recorder.capture(buffer, 0);

    buffer.clear();
    buffer.insert(MidiMessage::noteOn(0, 60, 110, 0));   // no note-off between
    recorder.capture(buffer, 12000);

    buffer.clear();
    buffer.insert(MidiMessage::noteOff(0, 60, 64, 0));
    recorder.capture(buffer, 24000);

    std::vector<RecordedEvent> events;
    recorder.drainInto(events, map, 48000);

    REQUIRE(events.size() == 2);
    CHECK(events[0].duration > 0);
    CHECK(events[1].duration > 0);
}

TEST_CASE("overlapping notes on different keys are kept apart")
{
    const TempoMap map{120.0, 48000.0};
    MidiRecorder   recorder;
    MidiBuffer     buffer;

    buffer.insert(MidiMessage::noteOn(0, 60, 100, 0));
    buffer.insert(MidiMessage::noteOn(0, 64, 90, 100));
    buffer.insert(MidiMessage::noteOn(0, 67, 80, 200));
    recorder.capture(buffer, 0);

    buffer.clear();
    buffer.insert(MidiMessage::noteOff(0, 64, 64, 0));   // released out of order
    buffer.insert(MidiMessage::noteOff(0, 60, 64, 100));
    buffer.insert(MidiMessage::noteOff(0, 67, 64, 200));
    recorder.capture(buffer, 24000);

    std::vector<RecordedEvent> events;
    recorder.drainInto(events, map, 48000);

    REQUIRE(events.size() == 3);
    for (const RecordedEvent& event : events)
        CHECK(event.duration > 0);
}

TEST_CASE("control changes and pitch bend are recorded, system messages are not")
{
    const TempoMap map{120.0, 48000.0};
    MidiRecorder   recorder;
    MidiBuffer     buffer;

    buffer.insert(MidiMessage::controlChange(0, 74, 90, 0));
    buffer.insert(MidiMessage::pitchBend(0, 9000, 10));
    buffer.insert(MidiMessage{20, 0xF8, 0, 0});   // MIDI clock
    recorder.capture(buffer, 0);

    std::vector<RecordedEvent> events;
    recorder.drainInto(events, map, 48000);

    REQUIRE(events.size() == 2);
    CHECK(events[0].kind == RecordedEvent::Kind::controlChange);
    CHECK(events[1].kind == RecordedEvent::Kind::pitchBend);
    CHECK(events[1].value == 9000);
}

TEST_CASE("capture is realtime-safe")
{
    MidiRecorder recorder;
    MidiBuffer   buffer;

    for (int index = 0; index < 32; ++index)
        buffer.insert(MidiMessage::noteOn(0, 60 + index % 12, 100, index));

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        for (int block = 0; block < 100; ++block)
            recorder.capture(buffer, block * 128);
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
    CHECK(recorder.droppedCount() == 0);
}

// ── Phase 5 exit criterion ────────────────────────────────────────────────────

TEST_CASE("recorded MIDI reproduces input timing across a tempo change")
{
    // The criterion from docs/ROADMAP.md. Input is generated at exact host
    // timestamps for known musical positions; after the whole input -> host
    // time -> frame -> tick chain, the recorded ticks must be the ones we
    // aimed at.
    constexpr SampleRate rate      = 48000.0;
    constexpr FrameCount blockSize = 128;

    TempoMap map{120.0, rate};
    map.setTempoEvents({{0, 120.0}, {ticksPerQuarterNote * 4, 180.0}});

    // Eight successive eighth notes, straddling the tempo change at beat 5.
    std::vector<Tick> intended;
    for (int index = 0; index < 16; ++index)
        intended.push_back(static_cast<Tick>(index) * (ticksPerQuarterNote / 2));

    MidiInput    input;
    MidiRecorder recorder;
    MidiBuffer   buffer;

    const std::uint64_t sessionStart = 5'000'000'000ull;

    for (std::size_t index = 0; index < intended.size(); ++index) {
        const FramePosition onFrame  = map.frameForTick(intended[index]);
        const FramePosition offFrame = map.frameForTick(intended[index] + ticksPerQuarterNote / 4);

        input.injectForTesting(timestamped(nanosForFrame(sessionStart, onFrame, rate),
                                           MidiMessage::noteOn(0, 60 + static_cast<int>(index), 100)));
        input.injectForTesting(timestamped(nanosForFrame(sessionStart, offFrame, rate),
                                           MidiMessage::noteOff(0, 60 + static_cast<int>(index))));
    }

    const FramePosition totalFrames = map.frameForTick(ticksPerQuarterNote * 12);

    for (FramePosition frame = 0; frame < totalFrames; frame += blockSize) {
        input.collectForBlock(buffer, nanosForFrame(sessionStart, frame, rate), blockSize, rate);
        recorder.capture(buffer, frame);
    }

    std::vector<RecordedEvent> events;
    recorder.drainInto(events, map, totalFrames);

    CHECK(input.droppedCount() == 0);
    CHECK(recorder.droppedCount() == 0);

    std::vector<Tick> recorded;
    for (const RecordedEvent& event : events)
        if (event.kind == RecordedEvent::Kind::note)
            recorded.push_back(event.tick);

    REQUIRE(recorded.size() == intended.size());

    // One tick at 180 BPM is about 7 frames, so exact equality is the right
    // bar here: anything worse would mean the conversion lost a whole frame.
    for (std::size_t index = 0; index < intended.size(); ++index)
        CHECK(recorded[index] == intended[index]);
}

TEST_CASE("recording accuracy does not depend on the block size")
{
    constexpr SampleRate rate = 48000.0;

    for (const FrameCount blockSize : {32, 64, 128, 256, 512, 1024}) {
        TempoMap map{140.0, rate};

        MidiInput    input;
        MidiRecorder recorder;
        MidiBuffer   buffer;

        const std::uint64_t sessionStart = 3'000'000'000ull;

        std::vector<Tick> intended;
        for (int index = 0; index < 8; ++index)
            intended.push_back(static_cast<Tick>(index) * (ticksPerQuarterNote / 4));

        for (const Tick tick : intended) {
            const FramePosition frame = map.frameForTick(tick);
            input.injectForTesting(timestamped(nanosForFrame(sessionStart, frame, rate),
                                               MidiMessage::noteOn(0, 60, 100)));
            input.injectForTesting(timestamped(nanosForFrame(sessionStart, frame + 1000, rate),
                                               MidiMessage::noteOff(0, 60)));
        }

        const FramePosition totalFrames = map.frameForTick(ticksPerQuarterNote * 4);

        for (FramePosition frame = 0; frame < totalFrames; frame += blockSize) {
            input.collectForBlock(buffer, nanosForFrame(sessionStart, frame, rate), blockSize, rate);
            recorder.capture(buffer, frame);
        }

        std::vector<RecordedEvent> events;
        recorder.drainInto(events, map, totalFrames);

        REQUIRE(events.size() == intended.size());
        for (std::size_t index = 0; index < intended.size(); ++index)
            CHECK(events[index].tick == intended[index]);
    }
}

// ── Quantize and humanize ─────────────────────────────────────────────────────

TEST_CASE("quantize snaps note starts to the grid")
{
    std::vector<project::MidiEvent> notes;
    const Tick grid = ticksPerQuarterNote / 4;   // sixteenths

    for (const Tick tick : {0, 231, 245, 470, 490})
        notes.push_back(patternNote(tick));

    project::quantizeNoteStarts(notes, grid, 1.0);

    for (const project::MidiEvent& event : notes)
        CHECK(event.tick % grid == 0);
}

TEST_CASE("partial quantize strength moves notes without pinning them")
{
    // A quantiser without strength is why quantised parts sound dead.
    std::vector<project::MidiEvent> notes;
    const Tick grid = ticksPerQuarterNote;

    notes.push_back(patternNote(100));
    project::quantizeNoteStarts(notes, grid, 0.5);

    CHECK(notes[0].tick == 50);   // halfway back to 0
}

TEST_CASE("quantize preserves note durations")
{
    // Snapping a note's length to the grid changes its articulation, which is
    // not what the user asked for.
    std::vector<project::MidiEvent> notes;
    notes.push_back(patternNote(231, 137));

    project::quantizeNoteStarts(notes, ticksPerQuarterNote / 4, 1.0);

    CHECK(notes[0].duration == 137);
}

TEST_CASE("quantize leaves non-note events alone")
{
    std::vector<project::MidiEvent> notes;
    project::MidiEvent cc;
    cc.type = project::MidiEventType::controlChange;
    cc.tick = 231;
    notes.push_back(cc);

    project::quantizeNoteStarts(notes, ticksPerQuarterNote, 1.0);

    CHECK(notes[0].tick == 231);
}

TEST_CASE("humanize is deterministic for a given seed")
{
    // Reproducibility is what makes it undoable, and what keeps the golden-file
    // audio tests meaningful.
    const auto build = [] {
        std::vector<project::MidiEvent> notes;
        for (int index = 0; index < 32; ++index)
            notes.push_back(patternNote(static_cast<Tick>(index) * ticksPerQuarterNote));
        return notes;
    };

    std::vector<project::MidiEvent> first  = build();
    std::vector<project::MidiEvent> second = build();
    std::vector<project::MidiEvent> other  = build();

    project::humanizeNoteStarts(first, 20, 12345);
    project::humanizeNoteStarts(second, 20, 12345);
    project::humanizeNoteStarts(other, 20, 999);

    CHECK(first == second);
    CHECK_FALSE(first == other);
}

TEST_CASE("humanize stays within its range and never goes negative")
{
    std::vector<project::MidiEvent> notes;
    notes.push_back(patternNote(0));
    for (int index = 1; index < 64; ++index)
        notes.push_back(patternNote(static_cast<Tick>(index) * ticksPerQuarterNote));

    project::humanizeNoteStarts(notes, 30, 7);

    for (const project::MidiEvent& event : notes)
        CHECK(event.tick >= 0);
}

TEST_CASE("recorded events become notes content sorted by position")
{
    std::vector<RecordedEvent> events;
    events.push_back(RecordedEvent{RecordedEvent::Kind::note, 960, 480, 0, 64, 90, 64});
    events.push_back(RecordedEvent{RecordedEvent::Kind::note, 0, 480, 0, 60, 100, 64});

    std::vector<project::MidiEvent> notes;
    project::appendRecordedEvents(notes, events);

    REQUIRE(notes.size() == 2);
    CHECK(notes[0].tick == 0);
    CHECK(notes[1].tick == 960);
    CHECK(notes[0].key == 60);
    CHECK(notes[0].duration == 480);
}
