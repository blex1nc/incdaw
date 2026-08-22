#pragma once

#include "engine/core/LockFreeQueue.h"
#include "engine/midi/MidiBuffer.h"
#include "platform/MidiDevice.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

namespace incdaw::platform {
class MidiDevice;
}

namespace incdaw::engine {

/// The audio thread's route out to an external MIDI destination.
///
/// The mirror image of MidiInput, and it exists for the same reason. MidiInput
/// turns a host timestamp into a frame offset so that a played note lands on
/// the sample it was played on; MidiOutput turns a frame offset back into a
/// host timestamp so that a note INCDAW generates is *heard* on the sample it
/// was written for. Sending everything at the moment the block is rendered
/// would quantise output to the buffer size — the same 2.7 ms at 128 frames
/// that the input path goes to such lengths to avoid.
///
/// The audio thread does not talk to CoreMIDI. Its send path allocates and
/// takes locks, either of which in a callback is a dropout. Messages cross a
/// lock-free queue to a sender thread, which hands them to the system with a
/// timestamp in the near future; the system is what delivers them on time.
/// That is the same division of labour the input side uses, run backwards.
///
/// The sender runs a block or so ahead of the deadline it is scheduling for,
/// so its polling interval costs no accuracy: what matters is that a message
/// reaches CoreMIDI before the time written on it, not how promptly the thread
/// woke up.
class MidiOutput {
public:
    /// Deep enough that a dense chord plus a stream of clock ticks cannot fill
    /// it between two sender wake-ups.
    static constexpr std::size_t queueCapacity = 2048;

    /// How long the sender sleeps between drains when idle.
    static constexpr int drainIntervalMillis = 1;

    MidiOutput() = default;
    ~MidiOutput();

    MidiOutput(const MidiOutput&)            = delete;
    MidiOutput& operator=(const MidiOutput&) = delete;

    /// Installs the destination, or clears it with nullptr.
    ///
    /// Blocks until the sender is not mid-send, so the caller may destroy the
    /// device immediately after clearing. Never called from the audio thread.
    void setDevice(platform::MidiDevice* device);

    [[nodiscard]] bool hasDevice() const noexcept { return active_.load(std::memory_order_acquire); }

    /// Starts and stops the sender thread. Idempotent.
    void start();
    void stop();

    [[nodiscard]] bool isRunning() const noexcept { return running_.load(std::memory_order_acquire); }

    /// Audio thread. Queues every message in `messages`, timestamped for the
    /// host time at which its frame will be heard.
    ///
    /// A no-op with no device attached, so a session with no external gear
    /// costs one atomic load per block.
    void sendForBlock(const MidiBuffer& messages,
                      std::uint64_t     blockHostTimeNanos,
                      SampleRate        sampleRate) noexcept;

    /// Audio thread. One message whose host time is already resolved.
    /// Returns false if the queue was full.
    bool send(const platform::TimestampedMidiMessage& message) noexcept;

    /// Hands everything queued to the device. Called by the sender thread;
    /// public so a test can drive the drain without a thread or a clock.
    /// Returns how many messages were sent.
    std::size_t drainPending();

    /// An "all notes off" and "sustain off" on every channel, sent as soon as
    /// possible.
    ///
    /// A transport stopped mid-note leaves an external synthesiser holding it
    /// forever — the note-off was scheduled for a block that is never rendered.
    /// This is what makes stop mean stop on hardware that has no idea the
    /// transport exists.
    void sendAllNotesOff() noexcept;

    [[nodiscard]] std::uint64_t sentCount()    const noexcept { return sent_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t droppedCount() const noexcept { return dropped_.load(std::memory_order_relaxed); }

    void resetCounters() noexcept;

private:
    void senderLoop();

    LockFreeQueue<platform::TimestampedMidiMessage, queueCapacity> queue_;

    /// Guards `device_` against a send in flight. Held only by the sender
    /// thread and by setDevice — never by the audio thread, which reads
    /// `active_` instead.
    mutable std::mutex      deviceMutex_;
    platform::MidiDevice*   device_ = nullptr;
    std::atomic<bool>       active_{false};

    std::thread             sender_;
    std::atomic<bool>       running_{false};
    std::mutex              wakeMutex_;
    std::condition_variable wake_;

    std::atomic<std::uint64_t> sent_{0};
    std::atomic<std::uint64_t> dropped_{0};
};

} // namespace incdaw::engine
