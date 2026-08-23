#pragma once

#include "engine/core/Time.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace incdaw::engine {

/// What an MPE event says about one note.
enum class MpeEventType : std::uint8_t {
    noteOn,
    noteOff,

    /// Per-note pitch, in semitones from the note's own key. Carries the
    /// member channel's bend scaled by the zone's bend range, which is why it
    /// is semitones here rather than a 14-bit number nobody downstream could
    /// interpret without knowing the zone.
    pitch,

    /// Per-note pressure, 0..1. Channel pressure on the member channel.
    pressure,

    /// Per-note timbre, 0..1. CC 74, the "third dimension" — MPE's own name
    /// for it, and deliberately not called "brightness": what it controls is
    /// the instrument's decision, not the protocol's.
    timbre,
};

/// One note's worth of expression, resolved.
///
/// Separate from `MidiMessage` rather than folded into it. MidiMessage is the
/// transport representation and sits in every buffer in the engine; widening it
/// with four expression fields would cost that memory everywhere to serve the
/// one path that needs it. This stream runs beside it and is empty — free —
/// unless a zone is configured.
///
/// The `noteId` is what makes this MPE rather than "MIDI on several channels".
/// A member channel is reused as soon as its note ends, so channel-and-key does
/// not identify a note across time; two notes on the same key in quick
/// succession are one note to a listener keyed on the pair, and the second
/// one's expression lands on the first one's voice.
struct MpeNoteEvent {
    FrameCount    frameOffset = 0;
    std::uint32_t noteId      = 0;
    MpeEventType  type        = MpeEventType::noteOn;

    std::uint8_t channel  = 0;   ///< the member channel the note lives on
    std::uint8_t key      = 0;
    std::uint8_t velocity = 0;   ///< note on and note off only

    /// Semitones for `pitch`, 0..1 for `pressure` and `timbre`, unused
    /// otherwise.
    float value = 0.0f;
};

static_assert(std::is_trivially_copyable_v<MpeNoteEvent>,
              "MPE events cross the realtime boundary and must be trivially copyable.");

/// One block's MPE events, in the order they were decoded.
///
/// Fixed capacity, no allocation: filled on the audio thread. Deliberately not
/// sorted on insert the way MidiBuffer is — the source is already ordered by
/// frame, and one incoming message can expand into several events that must
/// stay in the order they were derived (a note-on before the pitch that
/// belongs to it).
template <std::size_t Capacity = 512>
class BasicMpeEventBuffer {
public:
    bool append(const MpeNoteEvent& event) noexcept
    {
        if (count_ >= Capacity) {
            ++overflowed_;
            return false;
        }

        events_[count_++] = event;
        return true;
    }

    void clear() noexcept { count_ = 0; }

    [[nodiscard]] std::size_t size()    const noexcept { return count_; }
    [[nodiscard]] bool        isEmpty() const noexcept { return count_ == 0; }

    [[nodiscard]] const MpeNoteEvent& operator[](std::size_t index) const noexcept
    {
        return events_[index];
    }

    [[nodiscard]] const MpeNoteEvent* begin() const noexcept { return events_.data(); }
    [[nodiscard]] const MpeNoteEvent* end()   const noexcept { return events_.data() + count_; }

    [[nodiscard]] std::size_t overflowCount() const noexcept { return overflowed_; }
    void resetOverflowCount() noexcept { overflowed_ = 0; }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    std::array<MpeNoteEvent, Capacity> events_{};
    std::size_t                        count_      = 0;
    std::size_t                        overflowed_ = 0;
};

using MpeEventBuffer = BasicMpeEventBuffer<512>;

} // namespace incdaw::engine
