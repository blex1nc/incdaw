#pragma once

#include "engine/midi/MidiBuffer.h"
#include "engine/transport/TempoMap.h"

#include <cstdint>
#include <vector>

namespace incdaw::engine {

/// A note as the playback engine sees it.
///
/// Engine-level, like RecordedEvent and for the same reason: `engine/` sits
/// below `project/` and must not know about patterns, labels or per-note
/// properties. The project layer compiles its patterns into this.
struct SequencedNote {
    Tick startTick  = 0;
    Tick lengthTicks = 0;
    int  channel    = 0;
    int  key        = 60;
    int  velocity   = 100;

    [[nodiscard]] Tick endTick() const noexcept { return startTick + (lengthTicks > 0 ? lengthTicks : 1); }
};

/// Sorted note data, read by the audio thread to produce MIDI for a block.
///
/// Two orderings are kept: by start, for finding the note-ons in a range, and
/// by end, for the note-offs. Without the second, every block would have to
/// scan every note to discover which ones stop — O(n) per block, on the audio
/// thread, for the whole pattern.
class NoteSequence {
public:
    /// Off the audio thread. May allocate.
    void setNotes(std::vector<SequencedNote> notes);

    void clear() noexcept;

    [[nodiscard]] std::size_t noteCount() const noexcept { return notes_.size(); }
    [[nodiscard]] const std::vector<SequencedNote>& notes() const noexcept { return notes_; }

    /// Length of the sequence in ticks — the end of its last note, rounded up
    /// to `loopLengthTicks` when one is set.
    [[nodiscard]] Tick lengthTicks() const noexcept { return length_; }
    void setLoopLength(Tick ticks) noexcept;

    /// Appends the note-ons and note-offs falling inside a frame range.
    ///
    /// Realtime-safe: binary searches plus a bounded emit, no allocation.
    /// Offsets are relative to `blockStartFrame`.
    void collectForRange(MidiBuffer&   destination,
                         FramePosition blockStartFrame,
                         FrameCount    frameCount,
                         const TempoMap& tempoMap) const noexcept;

private:
    void rebuildIndices();

    std::vector<SequencedNote> notes_;          ///< sorted by startTick
    std::vector<std::uint32_t> byEnd_;          ///< indices into notes_, sorted by endTick
    Tick                       length_ = 0;
    Tick                       loopLength_ = 0;
};

} // namespace incdaw::engine
