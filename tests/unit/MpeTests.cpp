// C4 — MPE input.
//
// The representation has been called MPE-ready since Phase 5 and nothing
// decoded it: an MPE controller arrived as fifteen channels of unrelated
// monophonic keyboard, every glide read as a whole-zone bend, and pressure
// landing on whichever notes shared a channel. What the decoder adds is the
// association between a member channel's expression and the note living on it
// — and the note id, because the channel is reused the moment the note ends.

#include "doctest.h"

#include "engine/core/RealtimeGuard.h"
#include "engine/midi/MidiBuffer.h"
#include "app/AppSettings.h"
#include "engine/midi/MpeDecoder.h"

#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

MidiBuffer blockOf(std::initializer_list<MidiMessage> messages)
{
    MidiBuffer buffer;
    for (const MidiMessage& message : messages)
        (void)buffer.insert(message);
    return buffer;
}

MidiMessage channelPressure(int channel, int value, FrameCount offset = 0)
{
    return {offset, static_cast<std::uint8_t>(0xD0 | (channel & 0x0F)),
            static_cast<std::uint8_t>(value & 0x7F), 0};
}

/// The three-message gesture a controller uses to configure a zone.
std::vector<MidiMessage> configurationMessage(int channel, int members)
{
    return {MidiMessage::controlChange(channel, 101, 0),
            MidiMessage::controlChange(channel, 100, 6),
            MidiMessage::controlChange(channel, 6, members)};
}

/// Configures a decoder with a lower zone. Not a factory returning one by
/// value: the decoder holds atomics and is deliberately not copyable.
void configureLowerZone(MpeDecoder& decoder, int members = 7, double memberRange = 48.0)
{
    MpeZone lower;
    lower.kind                     = MpeZoneKind::lower;
    lower.memberChannelCount       = members;
    lower.memberPitchBendSemitones = memberRange;

    MpeZone upper;
    upper.kind = MpeZoneKind::upper;

    decoder.setZones(lower, upper);
}

/// The first event of a type, or nullptr.
const MpeNoteEvent* findEvent(const MpeEventBuffer& events, MpeEventType type,
                              std::uint32_t noteId = 0)
{
    for (const MpeNoteEvent& event : events)
        if (event.type == type && (noteId == 0 || event.noteId == noteId))
            return &event;

    return nullptr;
}

std::size_t countOf(const MpeEventBuffer& events, MpeEventType type)
{
    std::size_t count = 0;
    for (const MpeNoteEvent& event : events)
        if (event.type == type)
            ++count;

    return count;
}

} // namespace

// ── Off ──────────────────────────────────────────────────────────────────────

TEST_CASE("with no zone configured nothing is decoded")
{
    MpeDecoder     decoder;
    MpeEventBuffer events;

    CHECK_FALSE(decoder.isEnabled());

    decoder.decode(blockOf({MidiMessage::noteOn(1, 60, 100),
                            MidiMessage::pitchBend(1, 12000)}),
                   events);

    CHECK(events.isEmpty());
}

TEST_CASE("channel 1 is a master, not a member")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder);
    MpeEventBuffer events;

    // A note on the master channel is not a member note. MPE reserves the
    // master for zone-wide messages; treating it as a sixteenth voice is how
    // a decoder ends up with a note nothing can express on.
    decoder.decode(blockOf({MidiMessage::noteOn(0, 60, 100)}), events);

    CHECK(countOf(events, MpeEventType::noteOn) == 0);
}

// ── A note and its expression ────────────────────────────────────────────────

TEST_CASE("a member note arrives with its expression already set")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder);
    MpeEventBuffer events;

    // The order a real controller sends: shape the channel, then start the
    // note. A decoder that forwarded only changes would start the note flat
    // and correct it a moment later, which is audible.
    decoder.decode(blockOf({MidiMessage::pitchBend(1, 8192 + 4096, 0),
                            MidiMessage::controlChange(1, 74, 127, 0),
                            channelPressure(1, 64, 0),
                            MidiMessage::noteOn(1, 60, 100, 10)}),
                   events);

    const MpeNoteEvent* noteOn = findEvent(events, MpeEventType::noteOn);
    REQUIRE(noteOn != nullptr);
    CHECK(noteOn->key == 60);
    CHECK(noteOn->velocity == 100);
    CHECK(noteOn->channel == 1);
    CHECK(noteOn->frameOffset == 10);

    const std::uint32_t noteId = noteOn->noteId;
    CHECK(noteId != 0);

    const MpeNoteEvent* pitch = findEvent(events, MpeEventType::pitch, noteId);
    REQUIRE(pitch != nullptr);
    CHECK(pitch->value == doctest::Approx(24.0).epsilon(0.01));   // half of ±48

    const MpeNoteEvent* pressure = findEvent(events, MpeEventType::pressure, noteId);
    REQUIRE(pressure != nullptr);
    CHECK(pressure->value == doctest::Approx(64.0 / 127.0).epsilon(0.01));

    const MpeNoteEvent* timbre = findEvent(events, MpeEventType::timbre, noteId);
    REQUIRE(timbre != nullptr);
    CHECK(timbre->value == doctest::Approx(1.0));
}

TEST_CASE("a bend on a member channel moves only that channel's note")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder);
    MpeEventBuffer events;

    decoder.decode(blockOf({MidiMessage::noteOn(1, 60, 100),
                            MidiMessage::noteOn(2, 64, 100)}),
                   events);

    decoder.decode(blockOf({MidiMessage::pitchBend(2, 16383)}), events);

    // The whole point. Before this, a glide on one finger bent the chord.
    REQUIRE(countOf(events, MpeEventType::pitch) == 1);
    CHECK(events[0].channel == 2);
    CHECK(events[0].key == 64);
    CHECK(events[0].value == doctest::Approx(48.0).epsilon(0.01));
}

TEST_CASE("the pitch is reported in semitones, scaled by the zone's range")
{
    MpeEventBuffer events;

    MpeDecoder wide;
    configureLowerZone(wide, 7, 96.0);
    wide.decode(blockOf({MidiMessage::noteOn(1, 60, 100)}), events);
    wide.decode(blockOf({MidiMessage::pitchBend(1, 16383)}), events);
    REQUIRE(countOf(events, MpeEventType::pitch) == 1);
    const float atNinetySix = events[0].value;

    MpeDecoder narrow;
    configureLowerZone(narrow, 7, 2.0);
    narrow.decode(blockOf({MidiMessage::noteOn(1, 60, 100)}), events);
    narrow.decode(blockOf({MidiMessage::pitchBend(1, 16383)}), events);
    REQUIRE(countOf(events, MpeEventType::pitch) == 1);
    const float atTwo = events[0].value;

    // A controller using ±96 read as ±48 reports every glide at half size —
    // in tune with nothing, and consistently so.
    CHECK(atNinetySix == doctest::Approx(96.0).epsilon(0.01));
    CHECK(atTwo == doctest::Approx(2.0).epsilon(0.01));
}

TEST_CASE("the master channel bends the whole zone")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder);
    MpeEventBuffer events;

    decoder.decode(blockOf({MidiMessage::noteOn(1, 60, 100),
                            MidiMessage::noteOn(2, 64, 100),
                            MidiMessage::noteOn(3, 67, 100)}),
                   events);

    decoder.decode(blockOf({MidiMessage::pitchBend(0, 16383)}), events);

    REQUIRE(countOf(events, MpeEventType::pitch) == 3);

    // The master's own range, not the members'. Folding the two together
    // would make the result depend on which arrived last.
    for (const MpeNoteEvent& event : events)
        CHECK(event.value == doctest::Approx(2.0).epsilon(0.01));
}

// ── Note identity ────────────────────────────────────────────────────────────

TEST_CASE("a reused channel does not inherit the previous note's expression")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder);
    MpeEventBuffer events;

    decoder.decode(blockOf({MidiMessage::noteOn(1, 60, 100)}), events);
    const std::uint32_t first = events[0].noteId;

    decoder.decode(blockOf({MidiMessage::noteOff(1, 60),
                            MidiMessage::noteOn(1, 72, 100)}),
                   events);

    const MpeNoteEvent* second = findEvent(events, MpeEventType::noteOn);
    REQUIRE(second != nullptr);

    // Channel-and-key identifies a note only until the channel is reused,
    // which on MPE is immediately. Without an id, a bend meant for the second
    // note lands on a voice belonging to the first.
    CHECK(second->noteId != first);
    CHECK(second->key == 72);
}

TEST_CASE("a note off carries the id of the note it ends")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder);
    MpeEventBuffer events;

    decoder.decode(blockOf({MidiMessage::noteOn(1, 60, 100)}), events);
    const std::uint32_t noteId = events[0].noteId;

    decoder.decode(blockOf({MidiMessage::noteOff(1, 60, 40)}), events);

    REQUIRE(events.size() == 1);
    CHECK(events[0].type == MpeEventType::noteOff);
    CHECK(events[0].noteId == noteId);
    CHECK(events[0].velocity == 40);
}

TEST_CASE("expression after a note ends goes nowhere")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder);
    MpeEventBuffer events;

    decoder.decode(blockOf({MidiMessage::noteOn(1, 60, 100),
                            MidiMessage::noteOff(1, 60)}),
                   events);

    decoder.decode(blockOf({MidiMessage::pitchBend(1, 16383),
                            channelPressure(1, 100)}),
                   events);

    CHECK(events.isEmpty());
}

// ── Zones ────────────────────────────────────────────────────────────────────

TEST_CASE("the upper zone counts down from channel 16")
{
    MpeDecoder decoder;

    MpeZone lower;
    lower.kind = MpeZoneKind::lower;

    MpeZone upper;
    upper.kind               = MpeZoneKind::upper;
    upper.memberChannelCount = 4;   // channels 12..15 zero-based, master 16

    decoder.setZones(lower, upper);

    MpeEventBuffer events;
    decoder.decode(blockOf({MidiMessage::noteOn(14, 60, 100)}), events);
    CHECK(countOf(events, MpeEventType::noteOn) == 1);

    decoder.decode(blockOf({MidiMessage::noteOn(1, 60, 100)}), events);
    CHECK(countOf(events, MpeEventType::noteOn) == 0);   // not in any zone
}

TEST_CASE("a fifteen-member lower zone leaves no upper master")
{
    MpeDecoder decoder;
    configureLowerZone(decoder, 15);

    MpeEventBuffer events;
    decoder.decode(blockOf({MidiMessage::noteOn(15, 60, 100)}), events);

    // Channel 16 is a member here, not a master. Reading it as a master would
    // silently drop every note played on the top channel.
    CHECK(countOf(events, MpeEventType::noteOn) == 1);
}

TEST_CASE("a controller can configure its own zone")
{
    MpeDecoder     decoder;
    MpeEventBuffer events;

    decoder.setConfigurationMessagesHonoured(true);
    REQUIRE_FALSE(decoder.isEnabled());

    MidiBuffer buffer;
    for (const MidiMessage& message : configurationMessage(0, 7))
        (void)buffer.insert(message);

    decoder.decode(buffer, events);

    CHECK(decoder.isEnabled());
    CHECK(decoder.lowerZone().memberChannelCount == 7);
    CHECK(decoder.configurationCount() == 1);

    // And it takes effect immediately: a controller sends its configuration
    // and then starts playing, often in the same breath.
    decoder.decode(blockOf({MidiMessage::noteOn(1, 60, 100)}), events);
    CHECK(countOf(events, MpeEventType::noteOn) == 1);
}

TEST_CASE("zero member channels switches a zone off")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder);
    decoder.setConfigurationMessagesHonoured(true);
    MpeEventBuffer events;

    MidiBuffer buffer;
    for (const MidiMessage& message : configurationMessage(0, 0))
        (void)buffer.insert(message);

    decoder.decode(buffer, events);

    CHECK_FALSE(decoder.isEnabled());
    CHECK(decoder.lowerZone().memberChannelCount == 0);
}

TEST_CASE("configuration messages can be refused")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder, 7);
    MpeEventBuffer events;

    decoder.setConfigurationMessagesHonoured(false);

    MidiBuffer buffer;
    for (const MidiMessage& message : configurationMessage(0, 3))
        (void)buffer.insert(message);

    decoder.decode(buffer, events);

    CHECK(decoder.lowerZone().memberChannelCount == 7);
    CHECK(decoder.configurationCount() == 0);
}

TEST_CASE("only a master channel may configure a zone")
{
    MpeDecoder     decoder;
    decoder.setConfigurationMessagesHonoured(true);
    MpeEventBuffer events;

    MidiBuffer buffer;
    for (const MidiMessage& message : configurationMessage(5, 7))
        (void)buffer.insert(message);

    decoder.decode(buffer, events);

    CHECK_FALSE(decoder.isEnabled());
}

TEST_CASE("a controller can set its own pitch bend range")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder, 7, 48.0);
    MpeEventBuffer events;

    // RPN 0 on a member channel: the per-note range.
    decoder.decode(blockOf({MidiMessage::controlChange(1, 101, 0),
                            MidiMessage::controlChange(1, 100, 0),
                            MidiMessage::controlChange(1, 6, 96)}),
                   events);

    CHECK(decoder.lowerZone().memberPitchBendSemitones == doctest::Approx(96.0));

    // And on the master: the zone's own.
    decoder.decode(blockOf({MidiMessage::controlChange(0, 101, 0),
                            MidiMessage::controlChange(0, 100, 0),
                            MidiMessage::controlChange(0, 6, 12)}),
                   events);

    CHECK(decoder.lowerZone().masterPitchBendSemitones == doctest::Approx(12.0));
}

// ── Housekeeping ─────────────────────────────────────────────────────────────

TEST_CASE("reset forgets every sounding note")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder);
    MpeEventBuffer events;

    decoder.decode(blockOf({MidiMessage::noteOn(1, 60, 100)}), events);
    REQUIRE(decoder.noteCount() == 1);

    decoder.reset();
    CHECK(decoder.noteCount() == 0);

    // The note is gone, so its channel's expression has nothing to reach.
    decoder.decode(blockOf({MidiMessage::pitchBend(1, 16383)}), events);
    CHECK(events.isEmpty());
}

TEST_CASE("decoding a block allocates nothing")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder);
    MpeEventBuffer events;

    MidiBuffer buffer;
    for (int channel = 1; channel <= 7; ++channel) {
        (void)buffer.insert(MidiMessage::noteOn(channel, 60 + channel, 100, channel));
        (void)buffer.insert(MidiMessage::pitchBend(channel, 9000, channel * 2));
        (void)buffer.insert(channelPressure(channel, 90, channel * 3));
        (void)buffer.insert(MidiMessage::controlChange(channel, 74, 100, channel * 4));
    }

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        for (int index = 0; index < 32; ++index)
            decoder.decode(buffer, events);
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);

    if (!rt::guardEnabled())
        MESSAGE("realtime guard disabled in this build — allocation not verified");
}

TEST_CASE("a channel holding more notes than it should does not confuse the others")
{
    MpeDecoder     decoder;
    configureLowerZone(decoder);
    MpeEventBuffer events;

    MidiBuffer buffer;
    for (int index = 0; index < 10; ++index)
        (void)buffer.insert(MidiMessage::noteOn(1, 60 + index, 100, index));

    decoder.decode(buffer, events);

    // The convention is one note per member channel; a few are tolerated and
    // the rest are dropped rather than being mixed up with each other.
    CHECK(countOf(events, MpeEventType::noteOn) == MpeDecoder::notesPerChannel);

    decoder.decode(blockOf({MidiMessage::noteOn(2, 90, 100)}), events);
    CHECK(countOf(events, MpeEventType::noteOn) == 1);
}

TEST_CASE("a decoder with nothing configured is not worth a block")
{
    MpeDecoder decoder;

    // Off costs one atomic load per block rather than a pass over its MIDI,
    // which is what most sessions get: an ordinary keyboard, or nothing.
    CHECK_FALSE(decoder.isListening());

    decoder.setConfigurationMessagesHonoured(true);
    CHECK(decoder.isListening());

    decoder.setConfigurationMessagesHonoured(false);
    configureLowerZone(decoder);
    CHECK(decoder.isListening());
}

TEST_CASE("the MPE preference round-trips and defaults to off")
{
    app::AppSettings settings;
    CHECK_FALSE(settings.midiAcceptMpe);

    settings.midiAcceptMpe = true;
    CHECK(app::AppSettings::fromJson(settings.toJson()).midiAcceptMpe);

    // A settings file written before MPE existed reads as off.
    CHECK_FALSE(app::AppSettings::fromJson(R"({"midi":{"inputs":[]}})").midiAcceptMpe);
}
