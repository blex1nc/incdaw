#pragma once

#include "engine/core/LockFreeQueue.h"
#include "engine/midi/MidiBuffer.h"
#include "platform/MidiDevice.h"

#include <atomic>
#include <cstdint>

namespace incdaw::engine {

/// Bridges the system MIDI thread to the audio thread.
///
/// CoreMIDI delivers on its own high-priority thread; the audio thread must not
/// be handed anything from it directly. Messages cross through a lock-free
/// queue and are converted from host time to a frame offset inside the block
/// that is currently being rendered.
///
/// That conversion is the whole point. Placing every message at the start of
/// whichever block noticed it quantises input to the buffer size — 2.7 ms of
/// jitter at 128 frames, and 11 ms at 512. FL Studio's 2026 notes single out
/// reduced MIDI jitter as a headline improvement; this is what that means.
class MidiInput final : public platform::MidiInputCallback {
public:
    /// Deep enough that a dense chord plus pedal traffic cannot fill it between
    /// two audio callbacks.
    static constexpr std::size_t queueCapacity = 2048;

    /// Called on the MIDI thread. Realtime-safe.
    void midiMessageReceived(const platform::TimestampedMidiMessage& message) noexcept override;

    /// Called on the audio thread at the start of each block.
    ///
    /// `blockHostTimeNanos` is when the first frame of this block will be heard.
    /// Messages timestamped before it are clamped to offset 0 — they are already
    /// late, and the earliest audible moment is the best available answer.
    /// Messages timestamped beyond the block are held for the next one.
    void collectForBlock(MidiBuffer&   destination,
                         std::uint64_t blockHostTimeNanos,
                         FrameCount    frameCount,
                         SampleRate    sampleRate) noexcept;

    /// Messages dropped because the queue was full. Must be zero.
    [[nodiscard]] std::uint64_t droppedCount() const noexcept { return dropped_.load(std::memory_order_relaxed); }

    /// Messages that arrived timestamped in the past and were clamped.
    /// Nonzero is normal on a busy system; a large ratio means the audio
    /// callback is running late.
    [[nodiscard]] std::uint64_t lateCount() const noexcept { return late_.load(std::memory_order_relaxed); }

    [[nodiscard]] std::uint64_t receivedCount() const noexcept { return received_.load(std::memory_order_relaxed); }

    void resetCounters() noexcept;

    /// Feeds a message as if it had come from hardware. Used by the tests, which
    /// must be able to drive exact timings without a keyboard attached, and by
    /// the on-screen keyboard later.
    void injectForTesting(const platform::TimestampedMidiMessage& message) noexcept
    {
        midiMessageReceived(message);
    }

private:
    LockFreeQueue<platform::TimestampedMidiMessage, queueCapacity> queue_;

    /// One message read from the queue but belonging to a later block.
    ///
    /// A single slot suffices because the queue is time-ordered: if one message
    /// is too far in the future, every message behind it is too.
    platform::TimestampedMidiMessage pending_{};
    bool                             hasPending_ = false;

    std::atomic<std::uint64_t> received_{0};
    std::atomic<std::uint64_t> dropped_{0};
    std::atomic<std::uint64_t> late_{0};
};

} // namespace incdaw::engine
