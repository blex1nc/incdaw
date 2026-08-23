#pragma once

#include "engine/midi/MidiBuffer.h"
#include "engine/midi/MpeEvent.h"

#include <array>
#include <atomic>
#include <cstdint>

namespace incdaw::engine {

/// Which half of the keyboard a zone occupies.
///
/// MPE defines two, and they are not interchangeable: the lower zone's master
/// is channel 1 with members counting up from 2, the upper zone's master is
/// channel 16 with members counting DOWN from 15. A controller split into two
/// halves uses both at once.
enum class MpeZoneKind : std::uint8_t { lower, upper };

/// One MPE zone.
struct MpeZone {
    MpeZoneKind kind = MpeZoneKind::lower;

    /// Member channels in the zone. Zero means the zone is off — which is what
    /// an MPE Configuration Message carrying zero means, and why "off" is a
    /// count rather than a separate flag.
    int memberChannelCount = 0;

    /// Pitch bend range for a member channel. The MPE default is ±48
    /// semitones, chosen so a finger can slide the width of the keyboard; a
    /// decoder that assumed the ordinary ±2 would report a twenty-fourth of
    /// every glide.
    double memberPitchBendSemitones = 48.0;

    /// Pitch bend range for the master channel, which bends the whole zone.
    /// The ordinary default, because that is what it is.
    double masterPitchBendSemitones = 2.0;

    [[nodiscard]] bool isEnabled() const noexcept { return memberChannelCount > 0; }

    /// The zone's master channel, zero-based.
    [[nodiscard]] int masterChannel() const noexcept { return kind == MpeZoneKind::lower ? 0 : 15; }

    /// True if `channel` (zero-based) is one of this zone's member channels.
    [[nodiscard]] bool isMemberChannel(int channel) const noexcept
    {
        if (!isEnabled())
            return false;

        return kind == MpeZoneKind::lower
                   ? (channel >= 1 && channel <= memberChannelCount)
                   : (channel <= 14 && channel >= 15 - memberChannelCount);
    }
};

/// Turns a stream of ordinary MIDI into per-note expression.
///
/// The MIDI representation has been described as MPE-ready since Phase 5 and
/// nothing decoded it: an MPE controller reached INCDAW as fifteen channels of
/// unrelated monophonic keyboard, every glide reported as a whole-zone bend,
/// and pressure landing on whichever notes happened to share a channel.
///
/// What the decoder adds is the association. A member channel carries exactly
/// one note at a time by convention, so the channel's pitch bend, channel
/// pressure and CC 74 belong to *that* note — and the moment the note ends the
/// channel is reused, which is why every note gets an id rather than being
/// identified by the channel it happened to arrive on.
///
/// The zone can be set from Settings or by the controller itself: an MPE
/// Configuration Message (RPN 6) on channel 1 or 16 is how a controller
/// announces its own layout, and honouring it is the difference between
/// working when plugged in and working after being configured.
///
/// Realtime-safe. One pass over the block, bounded per-channel state, no
/// allocation. Costs nothing at all with no zone configured — the common case,
/// since MPE is off until something asks for it.
class MpeDecoder {
public:
    /// Notes one member channel may hold at once.
    ///
    /// The convention is one, and everything is built around that. A few more
    /// costs almost nothing and means a controller that briefly overlaps two
    /// notes on a channel — legato on a rotating allocator — does not silently
    /// lose one.
    static constexpr std::size_t notesPerChannel = 4;

    /// Both zones, set together. Non-realtime; the audio thread reads the
    /// result through atomics.
    void setZones(const MpeZone& lower, const MpeZone& upper) noexcept;

    [[nodiscard]] MpeZone lowerZone() const noexcept;
    [[nodiscard]] MpeZone upperZone() const noexcept;

    /// Whether a configuration message from the controller may change the
    /// zones.
    ///
    /// OFF by default, and that is a cost decision rather than a policy one: a
    /// decoder that might be configured at any moment has to examine every
    /// block, and most sessions have an ordinary keyboard attached or nothing
    /// at all. One checkbox turns it on, after which a controller announcing
    /// its own layout is all the configuration MPE needs.
    void setConfigurationMessagesHonoured(bool honoured) noexcept
    {
        honourMcm_.store(honoured, std::memory_order_relaxed);
    }

    [[nodiscard]] bool areConfigurationMessagesHonoured() const noexcept
    {
        return honourMcm_.load(std::memory_order_relaxed);
    }

    /// A zone exists.
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_.load(std::memory_order_relaxed); }

    /// Worth handing a block to: either a zone exists, or a controller is
    /// allowed to announce one. Anything else can skip the decoder entirely.
    [[nodiscard]] bool isListening() const noexcept
    {
        return isEnabled() || areConfigurationMessagesHonoured();
    }

    /// Forgets every sounding note and every channel's expression state.
    /// Called when the device restarts or the zones change.
    void reset() noexcept;

    /// Audio thread. Decodes one block's MIDI into `destination`.
    ///
    /// `destination` is cleared first. Messages that are not part of a zone
    /// pass through untouched — they are still in the MidiBuffer the
    /// instruments receive, and this stream is additional to it, not a
    /// replacement for it.
    void decode(const MidiBuffer& incoming, MpeEventBuffer& destination) noexcept;

    /// Notes started since the last reset. Diagnostics and tests.
    [[nodiscard]] std::uint32_t noteCount() const noexcept { return nextNoteId_; }

    /// Configuration messages honoured since the last reset.
    [[nodiscard]] std::uint32_t configurationCount() const noexcept
    {
        return configurations_.load(std::memory_order_relaxed);
    }

private:
    struct ActiveNote {
        std::uint32_t noteId = 0;
        std::uint8_t  key    = 0;
        bool          active = false;
    };

    struct ChannelState {
        std::array<ActiveNote, notesPerChannel> notes{};

        int bend14    = 8192;   ///< last pitch bend, centred
        int pressure  = 0;
        int timbre    = 64;     ///< CC 74 centres at 64, not at 0

        /// Registered parameter number being addressed, or -1.
        int rpn = -1;
    };

    void handleMemberMessage(const MidiMessage& message, const MpeZone& zone, int channel,
                             MpeEventBuffer& destination) noexcept;
    void handleMasterMessage(const MidiMessage& message, const MpeZone& zone,
                             MpeEventBuffer& destination) noexcept;
    void handleRpn(const MidiMessage& message, int channel, bool lower, bool isMaster) noexcept;

    void emitForChannel(int channel, MpeEventType type, float value, FrameCount offset,
                        MpeEventBuffer& destination) noexcept;
    void emitForZone(const MpeZone& zone, MpeEventType type, float value, FrameCount offset,
                     MpeEventBuffer& destination) noexcept;

    void publishEnabled() noexcept;

    std::array<ChannelState, 16> channels_{};

    /// Zones as plain values for the audio thread. Held field by field in
    /// atomics rather than as one struct: a lock-free atomic of a struct this
    /// size is not guaranteed, and a mutex here would be a lock on the audio
    /// thread.
    std::atomic<int>    lowerMembers_{0};
    std::atomic<int>    upperMembers_{0};
    std::atomic<double> lowerMemberRange_{48.0};
    std::atomic<double> upperMemberRange_{48.0};
    std::atomic<double> lowerMasterRange_{2.0};
    std::atomic<double> upperMasterRange_{2.0};

    std::atomic<bool> enabled_{false};
    std::atomic<bool> honourMcm_{false};
    std::atomic<std::uint32_t> configurations_{0};

    std::uint32_t nextNoteId_ = 0;
};

} // namespace incdaw::engine
