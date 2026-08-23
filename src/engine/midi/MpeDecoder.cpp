#include "engine/midi/MpeDecoder.h"

#include <algorithm>

namespace incdaw::engine {
namespace {

constexpr int rpnPitchBendSensitivity = 0x0000;
constexpr int rpnMpeConfiguration     = 0x0006;

constexpr int ccBankSelect      = 0;
constexpr int ccDataEntryMsb    = 6;
constexpr int ccTimbre          = 74;
constexpr int ccRpnLsb          = 100;
constexpr int ccRpnMsb          = 101;

/// A 14-bit bend, centred, as a fraction of full deflection.
constexpr float bendFraction(int bend14) noexcept
{
    const int offset = bend14 - 8192;
    return static_cast<float>(offset) / (offset < 0 ? 8192.0f : 8191.0f);
}

constexpr float sevenBitToUnit(int value) noexcept
{
    return static_cast<float>(std::clamp(value, 0, 127)) / 127.0f;
}

} // namespace

void MpeDecoder::setZones(const MpeZone& lower, const MpeZone& upper) noexcept
{
    lowerMembers_.store(std::clamp(lower.memberChannelCount, 0, 15), std::memory_order_relaxed);
    upperMembers_.store(std::clamp(upper.memberChannelCount, 0, 15), std::memory_order_relaxed);

    lowerMemberRange_.store(lower.memberPitchBendSemitones, std::memory_order_relaxed);
    upperMemberRange_.store(upper.memberPitchBendSemitones, std::memory_order_relaxed);
    lowerMasterRange_.store(lower.masterPitchBendSemitones, std::memory_order_relaxed);
    upperMasterRange_.store(upper.masterPitchBendSemitones, std::memory_order_relaxed);

    publishEnabled();
}

void MpeDecoder::publishEnabled() noexcept
{
    enabled_.store(lowerMembers_.load(std::memory_order_relaxed) > 0
                       || upperMembers_.load(std::memory_order_relaxed) > 0,
                   std::memory_order_relaxed);
}

MpeZone MpeDecoder::lowerZone() const noexcept
{
    MpeZone zone;
    zone.kind                     = MpeZoneKind::lower;
    zone.memberChannelCount       = lowerMembers_.load(std::memory_order_relaxed);
    zone.memberPitchBendSemitones = lowerMemberRange_.load(std::memory_order_relaxed);
    zone.masterPitchBendSemitones = lowerMasterRange_.load(std::memory_order_relaxed);
    return zone;
}

MpeZone MpeDecoder::upperZone() const noexcept
{
    MpeZone zone;
    zone.kind                     = MpeZoneKind::upper;
    zone.memberChannelCount       = upperMembers_.load(std::memory_order_relaxed);
    zone.memberPitchBendSemitones = upperMemberRange_.load(std::memory_order_relaxed);
    zone.masterPitchBendSemitones = upperMasterRange_.load(std::memory_order_relaxed);
    return zone;
}

void MpeDecoder::reset() noexcept
{
    channels_ = {};

    for (ChannelState& channel : channels_) {
        channel.bend14   = 8192;
        channel.timbre   = 64;
        channel.rpn      = -1;
    }

    nextNoteId_ = 0;
    configurations_.store(0, std::memory_order_relaxed);
}

void MpeDecoder::emitForChannel(int channel, MpeEventType type, float value, FrameCount offset,
                                MpeEventBuffer& destination) noexcept
{
    ChannelState& state = channels_[static_cast<std::size_t>(channel)];

    for (const ActiveNote& note : state.notes) {
        if (!note.active)
            continue;

        MpeNoteEvent event;
        event.frameOffset = offset;
        event.noteId      = note.noteId;
        event.type        = type;
        event.channel     = static_cast<std::uint8_t>(channel);
        event.key         = note.key;
        event.value       = value;

        (void)destination.append(event);
    }
}

void MpeDecoder::emitForZone(const MpeZone& zone, MpeEventType type, float value,
                             FrameCount offset, MpeEventBuffer& destination) noexcept
{
    for (int channel = 0; channel < 16; ++channel)
        if (zone.isMemberChannel(channel))
            emitForChannel(channel, type, value, offset, destination);
}

void MpeDecoder::handleRpn(const MidiMessage& message, int channel, bool lower,
                           bool isMaster) noexcept
{
    ChannelState& state = channels_[static_cast<std::size_t>(channel)];

    switch (message.data1) {
    case ccRpnMsb:
        state.rpn = state.rpn < 0 ? (message.data2 << 7) : ((message.data2 << 7) | (state.rpn & 0x7F));
        break;

    case ccRpnLsb:
        state.rpn = state.rpn < 0 ? message.data2 : ((state.rpn & 0x3F80) | message.data2);
        break;

    case ccDataEntryMsb: {
        if (state.rpn == rpnMpeConfiguration) {
            if (!honourMcm_.load(std::memory_order_relaxed))
                break;

            // The controller announcing its own layout. Channel 1 configures
            // the lower zone, channel 16 the upper; the value is the number of
            // member channels, and zero switches the zone off.
            const int members = std::clamp<int>(message.data2, 0, 15);

            if (channel == 0)
                lowerMembers_.store(members, std::memory_order_relaxed);
            else if (channel == 15)
                upperMembers_.store(members, std::memory_order_relaxed);
            else
                break;   // only a master channel may configure a zone

            publishEnabled();
            configurations_.fetch_add(1, std::memory_order_relaxed);
        } else if (state.rpn == rpnPitchBendSensitivity) {
            // Sent on a member channel it sets the per-note range, on a master
            // channel the zone's own. Both matter: a controller that uses ±96
            // and is read as ±48 reports every glide at half size.
            const auto semitones = static_cast<double>(message.data2);

            if (isMaster) {
                (lower ? lowerMasterRange_ : upperMasterRange_)
                    .store(semitones, std::memory_order_relaxed);
            } else {
                (lower ? lowerMemberRange_ : upperMemberRange_)
                    .store(semitones, std::memory_order_relaxed);
            }
        }
        break;
    }

    default:
        break;
    }
}

void MpeDecoder::handleMemberMessage(const MidiMessage& message, const MpeZone& zone, int channel,
                                     MpeEventBuffer& destination) noexcept
{
    ChannelState& state = channels_[static_cast<std::size_t>(channel)];

    if (message.isNoteOn()) {
        for (ActiveNote& note : state.notes) {
            if (note.active)
                continue;

            note.active = true;
            note.key    = static_cast<std::uint8_t>(message.noteNumber());
            note.noteId = ++nextNoteId_;

            MpeNoteEvent event;
            event.frameOffset = message.frameOffset;
            event.noteId      = note.noteId;
            event.type        = MpeEventType::noteOn;
            event.channel     = static_cast<std::uint8_t>(channel);
            event.key         = note.key;
            event.velocity    = static_cast<std::uint8_t>(message.velocity());
            (void)destination.append(event);

            // The channel's current expression belongs to the new note from
            // its first frame. MPE controllers send bend and timbre BEFORE the
            // note-on so the note starts already shaped; a decoder that only
            // forwarded changes would start every note flat and correct it
            // afterwards, which is audible as a click into position.
            const float bend =
                bendFraction(state.bend14) * static_cast<float>(zone.memberPitchBendSemitones);

            MpeNoteEvent pitch = event;
            pitch.type     = MpeEventType::pitch;
            pitch.velocity = 0;
            pitch.value    = bend;
            (void)destination.append(pitch);

            MpeNoteEvent pressure = pitch;
            pressure.type  = MpeEventType::pressure;
            pressure.value = sevenBitToUnit(state.pressure);
            (void)destination.append(pressure);

            MpeNoteEvent timbre = pitch;
            timbre.type  = MpeEventType::timbre;
            timbre.value = sevenBitToUnit(state.timbre);
            (void)destination.append(timbre);

            return;
        }

        return;   // the channel is full; the note is dropped rather than mixed up
    }

    if (message.isNoteOff()) {
        for (ActiveNote& note : state.notes) {
            if (!note.active || note.key != message.noteNumber())
                continue;

            MpeNoteEvent event;
            event.frameOffset = message.frameOffset;
            event.noteId      = note.noteId;
            event.type        = MpeEventType::noteOff;
            event.channel     = static_cast<std::uint8_t>(channel);
            event.key         = note.key;
            event.velocity    = static_cast<std::uint8_t>(message.velocity());
            (void)destination.append(event);

            note.active = false;
            return;
        }

        return;
    }

    if (message.isPitchBend()) {
        state.bend14 = message.pitchBendValue();

        emitForChannel(channel, MpeEventType::pitch,
                       bendFraction(state.bend14)
                           * static_cast<float>(zone.memberPitchBendSemitones),
                       message.frameOffset, destination);
        return;
    }

    if (message.type() == 0xD0) {   // channel pressure
        state.pressure = message.data1;
        emitForChannel(channel, MpeEventType::pressure, sevenBitToUnit(state.pressure),
                       message.frameOffset, destination);
        return;
    }

    if (message.isControlChange()) {
        if (message.data1 == ccTimbre) {
            state.timbre = message.data2;
            emitForChannel(channel, MpeEventType::timbre, sevenBitToUnit(state.timbre),
                           message.frameOffset, destination);
            return;
        }

        if (message.data1 == ccRpnMsb || message.data1 == ccRpnLsb
            || message.data1 == ccDataEntryMsb)
            handleRpn(message, channel, zone.kind == MpeZoneKind::lower, false);
    }
}

void MpeDecoder::handleMasterMessage(const MidiMessage& message, const MpeZone& zone,
                                     MpeEventBuffer& destination) noexcept
{
    const int channel = zone.masterChannel();

    ChannelState& state = channels_[static_cast<std::size_t>(channel)];

    if (message.isPitchBend()) {
        state.bend14 = message.pitchBendValue();

        // The master bends the whole zone, on top of whatever each note is
        // already doing. It is sent as its own event per note rather than
        // folded into the per-note pitch, because the two have different
        // ranges and folding them would make the result depend on which
        // arrived last.
        emitForZone(zone, MpeEventType::pitch,
                    bendFraction(state.bend14)
                        * static_cast<float>(zone.masterPitchBendSemitones),
                    message.frameOffset, destination);
        return;
    }

    if (message.type() == 0xD0) {
        state.pressure = message.data1;
        emitForZone(zone, MpeEventType::pressure, sevenBitToUnit(state.pressure),
                    message.frameOffset, destination);
        return;
    }

    if (message.isControlChange()) {
        if (message.data1 == ccTimbre) {
            state.timbre = message.data2;
            emitForZone(zone, MpeEventType::timbre, sevenBitToUnit(state.timbre),
                        message.frameOffset, destination);
            return;
        }

        if (message.data1 == ccRpnMsb || message.data1 == ccRpnLsb
            || message.data1 == ccDataEntryMsb || message.data1 == ccBankSelect)
            handleRpn(message, channel, zone.kind == MpeZoneKind::lower, true);
    }
}

void MpeDecoder::decode(const MidiBuffer& incoming, MpeEventBuffer& destination) noexcept
{
    destination.clear();

    // Re-read after every message rather than hoisted out of the loop: a
    // configuration message changes the layout mid-block, and the messages
    // behind it belong to the new one — a controller announces itself and
    // starts playing in the same breath.
    //
    // Master channels are examined even with no zone configured, because a
    // configuration message is how a zone comes into existence — a decoder
    // that only listened once it was already enabled could never be enabled
    // by the controller at all.
    MpeZone lower = lowerZone();
    MpeZone upper = upperZone();

    for (const MidiMessage& message : incoming) {
        if (message.isSystemMessage())
            continue;

        const int channel = message.channel();

        // Membership wins over the master role. A lower zone with fifteen
        // members occupies every channel from 2 to 16, which leaves channel 16
        // a member and no upper zone to be master of.
        if (lower.isMemberChannel(channel)) {
            handleMemberMessage(message, lower, channel, destination);
        } else if (upper.isMemberChannel(channel)) {
            handleMemberMessage(message, upper, channel, destination);
        } else if (channel == 0) {
            handleMasterMessage(message, lower, destination);
        } else if (channel == 15) {
            handleMasterMessage(message, upper, destination);
        }

        // A configuration message just changed the layout; the rest of the
        // block belongs to the new one.
        lower = lowerZone();
        upper = upperZone();
    }
}

} // namespace incdaw::engine
