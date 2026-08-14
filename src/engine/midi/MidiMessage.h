#pragma once

#include "engine/core/Time.h"

#include <cstdint>
#include <type_traits>

namespace incdaw::engine {

/// A MIDI message as the realtime path sees it.
///
/// Trivially copyable and free of ownership, because it crosses the realtime
/// boundary through a lock-free queue and the audio thread must never run a
/// destructor that frees memory (docs/ARCHITECTURE.md §4).
///
/// This is the *transport* representation. The editable, per-note-property
/// representation is `project::MidiEvent`; there is deliberately one of each
/// rather than one type trying to be both — a struct carrying a std::string
/// label cannot travel through a lock-free queue.
struct MidiMessage {
    /// Frame offset within the current block. This is what makes MIDI
    /// sample-accurate rather than block-accurate.
    FrameCount    frameOffset = 0;

    std::uint8_t  status = 0;
    std::uint8_t  data1  = 0;
    std::uint8_t  data2  = 0;

    // ── Construction ────────────────────────────────────────────────────────

    [[nodiscard]] static MidiMessage noteOn(int channel, int note, int velocity, FrameCount offset = 0) noexcept
    {
        // Velocity 0 on a note-on means note-off in the MIDI spec, and many
        // devices send it that way. Emitting it here would silently turn a
        // requested note-on into a note-off.
        return {offset, static_cast<std::uint8_t>(0x90 | (channel & 0x0F)),
                static_cast<std::uint8_t>(note & 0x7F),
                static_cast<std::uint8_t>(velocity < 1 ? 1 : (velocity > 127 ? 127 : velocity))};
    }

    [[nodiscard]] static MidiMessage noteOff(int channel, int note, int velocity = 64, FrameCount offset = 0) noexcept
    {
        return {offset, static_cast<std::uint8_t>(0x80 | (channel & 0x0F)),
                static_cast<std::uint8_t>(note & 0x7F),
                static_cast<std::uint8_t>(velocity & 0x7F)};
    }

    [[nodiscard]] static MidiMessage controlChange(int channel, int controller, int value, FrameCount offset = 0) noexcept
    {
        return {offset, static_cast<std::uint8_t>(0xB0 | (channel & 0x0F)),
                static_cast<std::uint8_t>(controller & 0x7F),
                static_cast<std::uint8_t>(value & 0x7F)};
    }

    [[nodiscard]] static MidiMessage pitchBend(int channel, int value14Bit, FrameCount offset = 0) noexcept
    {
        const int clamped = value14Bit < 0 ? 0 : (value14Bit > 16383 ? 16383 : value14Bit);
        return {offset, static_cast<std::uint8_t>(0xE0 | (channel & 0x0F)),
                static_cast<std::uint8_t>(clamped & 0x7F),
                static_cast<std::uint8_t>((clamped >> 7) & 0x7F)};
    }

    // ── Inspection ──────────────────────────────────────────────────────────

    [[nodiscard]] int type() const noexcept { return status & 0xF0; }
    [[nodiscard]] int channel() const noexcept { return status & 0x0F; }

    [[nodiscard]] bool isNoteOn() const noexcept { return type() == 0x90 && data2 > 0; }

    /// True for an explicit note-off *and* for a note-on with velocity 0, which
    /// the MIDI spec defines as equivalent. Treating them differently is the
    /// classic source of stuck notes.
    [[nodiscard]] bool isNoteOff() const noexcept
    {
        return type() == 0x80 || (type() == 0x90 && data2 == 0);
    }

    [[nodiscard]] bool isNote()          const noexcept { return type() == 0x80 || type() == 0x90; }
    [[nodiscard]] bool isControlChange() const noexcept { return type() == 0xB0; }
    [[nodiscard]] bool isPitchBend()     const noexcept { return type() == 0xE0; }

    /// Real-time messages (clock, start, stop) are not channel messages and
    /// must not be routed or transposed as if they were.
    [[nodiscard]] bool isSystemMessage() const noexcept { return status >= 0xF0; }

    [[nodiscard]] int noteNumber() const noexcept { return data1; }
    [[nodiscard]] int velocity()   const noexcept { return data2; }

    [[nodiscard]] int pitchBendValue() const noexcept { return data1 | (data2 << 7); }

    [[nodiscard]] friend bool operator==(const MidiMessage&, const MidiMessage&) noexcept = default;
};

static_assert(std::is_trivially_copyable_v<MidiMessage>,
              "MidiMessage crosses the realtime boundary and must be trivially copyable.");

} // namespace incdaw::engine
