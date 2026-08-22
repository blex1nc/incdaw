#include "engine/midi/MidiOutput.h"

#include "platform/HostTime.h"

#include <chrono>

namespace incdaw::engine {
namespace {

constexpr double nanosPerSecond = 1'000'000'000.0;

} // namespace

MidiOutput::~MidiOutput()
{
    stop();
}

void MidiOutput::setDevice(platform::MidiDevice* device)
{
    // Cleared before the lock is taken so the audio thread stops queueing
    // immediately, rather than filling the queue for a device that is about to
    // go away.
    if (device == nullptr)
        active_.store(false, std::memory_order_release);

    {
        const std::lock_guard<std::mutex> guard(deviceMutex_);
        device_ = device;
    }

    active_.store(device != nullptr, std::memory_order_release);
}

void MidiOutput::start()
{
    if (running_.exchange(true, std::memory_order_acq_rel))
        return;

    sender_ = std::thread([this] { senderLoop(); });
}

void MidiOutput::stop()
{
    if (!running_.exchange(false, std::memory_order_acq_rel))
        return;

    wake_.notify_all();

    if (sender_.joinable())
        sender_.join();
}

void MidiOutput::sendForBlock(const MidiBuffer& messages, std::uint64_t blockHostTimeNanos,
                              SampleRate sampleRate) noexcept
{
    if (!active_.load(std::memory_order_acquire) || messages.isEmpty() || sampleRate <= 0.0)
        return;

    const double nanosPerFrame = nanosPerSecond / sampleRate;

    for (const MidiMessage& message : messages) {
        platform::TimestampedMidiMessage timed;

        // The inverse of MidiInput's placement: the frame this message was
        // written for is heard `frameOffset` frames after the block's first
        // one, and that instant is the deadline the system is given.
        timed.hostTimeNanos =
            blockHostTimeNanos
            + static_cast<std::uint64_t>(static_cast<double>(message.frameOffset) * nanosPerFrame + 0.5);
        timed.status = message.status;
        timed.data1  = message.data1;
        timed.data2  = message.data2;

        if (!queue_.push(timed))
            dropped_.fetch_add(1, std::memory_order_relaxed);
    }
}

bool MidiOutput::send(const platform::TimestampedMidiMessage& message) noexcept
{
    if (!active_.load(std::memory_order_acquire))
        return false;

    if (queue_.push(message))
        return true;

    dropped_.fetch_add(1, std::memory_order_relaxed);
    return false;
}

std::size_t MidiOutput::drainPending()
{
    const std::lock_guard<std::mutex> guard(deviceMutex_);

    platform::TimestampedMidiMessage message;
    std::size_t                      count = 0;

    while (queue_.pop(message)) {
        // Still drained with no device so that a queue filled in the instant
        // before the device was cleared does not arrive at the next one.
        if (device_ != nullptr) {
            device_->sendMessage(message);
            ++count;
        }
    }

    sent_.fetch_add(count, std::memory_order_relaxed);
    return count;
}

void MidiOutput::sendAllNotesOff() noexcept
{
    if (!active_.load(std::memory_order_acquire))
        return;

    for (int channel = 0; channel < 16; ++channel) {
        platform::TimestampedMidiMessage message;
        message.hostTimeNanos = 0;   // as soon as possible: this is a rescue, not a performance
        message.status        = static_cast<std::uint8_t>(0xB0 | channel);

        message.data1 = 64;          // sustain pedal, released first so the notes can actually stop
        message.data2 = 0;
        if (!queue_.push(message))
            dropped_.fetch_add(1, std::memory_order_relaxed);

        message.data1 = 123;         // all notes off
        message.data2 = 0;
        if (!queue_.push(message))
            dropped_.fetch_add(1, std::memory_order_relaxed);
    }

    wake_.notify_all();
}

void MidiOutput::resetCounters() noexcept
{
    sent_.store(0, std::memory_order_relaxed);
    dropped_.store(0, std::memory_order_relaxed);
}

void MidiOutput::senderLoop()
{
    while (running_.load(std::memory_order_acquire)) {
        (void)drainPending();

        std::unique_lock<std::mutex> lock(wakeMutex_);
        wake_.wait_for(lock, std::chrono::milliseconds(drainIntervalMillis),
                       [this] { return !running_.load(std::memory_order_acquire); });
    }

    // One last pass so a note-off queued during shutdown still reaches the
    // device. Leaving it behind is a note held on external hardware with
    // nothing left running to release it.
    (void)drainPending();
}

} // namespace incdaw::engine
