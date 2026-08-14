#pragma once

#include "engine/core/LockFreeQueue.h"
#include "engine/midi/MidiBuffer.h"
#include "engine/transport/TempoMap.h"

#include <atomic>
#include <cstdint>
#include <vector>

namespace incdaw::engine {

/// A recorded musical event, in engine terms.
///
/// The recorder deliberately does NOT produce `project::MidiEvent`: `engine/`
/// sits below `project/` and must not know it exists (docs/ARCHITECTURE.md §2).
/// The project layer converts these into pattern events, which is also where
/// per-note properties like labels and probability get attached.
struct RecordedEvent {
    enum class Kind { note, controlChange, pitchBend };

    Kind kind         = Kind::note;
    Tick tick         = 0;
    Tick duration     = 0;
    int  channel      = 0;
    int  key          = 60;
    int  value        = 100;
    int  releaseValue = 64;
};

/// Captures incoming MIDI into a pattern.
///
/// Split across the realtime boundary the same way everything else is: the
/// audio thread records raw (frame, message) pairs into a lock-free queue, and
/// a non-realtime thread turns them into notes with durations.
///
/// Note pairing is deliberately NOT done on the audio thread. It needs a map of
/// sounding notes, and a note that is never released — the player lifts their
/// hand after recording stops, or a cable is pulled — would leave state the
/// audio thread has to reason about mid-callback.
class MidiRecorder {
public:
    static constexpr std::size_t queueCapacity = 8192;

    struct CapturedMessage {
        FramePosition frame  = 0;
        std::uint8_t  status = 0;
        std::uint8_t  data1  = 0;
        std::uint8_t  data2  = 0;
    };

    /// Audio thread. Records every message in `buffer` at its absolute frame.
    void capture(const MidiBuffer& buffer, FramePosition blockStartFrame) noexcept;

    /// Non-realtime. Converts everything captured so far into `events`,
    /// pairing note-ons with their note-offs through `tempoMap`.
    ///
    /// A note still held when recording ends is closed at `endFrame` rather than
    /// discarded: a truncated note is a recoverable edit, a lost one is not.
    void drainInto(std::vector<RecordedEvent>& events, const TempoMap& tempoMap, FramePosition endFrame);

    [[nodiscard]] std::uint64_t droppedCount() const noexcept { return dropped_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t capturedCount() const noexcept { return captured_.load(std::memory_order_relaxed); }

    void reset() noexcept;

private:
    LockFreeQueue<CapturedMessage, queueCapacity> queue_;
    std::atomic<std::uint64_t>                    dropped_{0};
    std::atomic<std::uint64_t>                    captured_{0};
};

} // namespace incdaw::engine
