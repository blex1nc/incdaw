#include "engine/midi/MidiInput.h"

namespace incdaw::engine {
namespace {

constexpr double nanosPerSecond = 1'000'000'000.0;

} // namespace

void MidiInput::midiMessageReceived(const platform::TimestampedMidiMessage& message) noexcept
{
    received_.fetch_add(1, std::memory_order_relaxed);

    // The learn tap: publish the last CC seen, packed into one atomic so a
    // reader can never tear it. Realtime-safe — one load, one store.
    if ((message.status & 0xF0u) == 0xB0u) {
        const std::uint64_t generation = (lastControl_.load(std::memory_order_relaxed) >> 24) + 1;
        const std::uint64_t packed =
            (generation << 24)
            | (static_cast<std::uint64_t>(message.status & 0x0Fu) << 16)
            | (static_cast<std::uint64_t>(message.data1) << 8)
            | static_cast<std::uint64_t>(message.data2);
        lastControl_.store(packed, std::memory_order_relaxed);
    }

    // The pad tap, alongside the learn tap above and for the same reason: a
    // watcher outside the audio thread needs to see presses. A SECOND queue
    // rather than a tap on the audio one, so a watcher and the audio thread
    // cannot take messages from each other.
    const auto kind = static_cast<std::uint8_t>(message.status & 0xF0u);

    if (kind == 0x90u || kind == 0x80u) {
        ObservedNote note;
        note.hostTimeNanos = message.hostTimeNanos;
        note.channel       = static_cast<int>(message.status & 0x0Fu);
        note.note          = static_cast<int>(message.data1);
        note.velocity      = static_cast<int>(message.data2);

        // A note-on at velocity zero is a note-off; every controller that ever
        // sent running status relies on it.
        note.on = kind == 0x90u && message.data2 > 0;

        if (!notes_.push(note))
            unobserved_.fetch_add(1, std::memory_order_relaxed);
    }

    if (!queue_.push(message))
        dropped_.fetch_add(1, std::memory_order_relaxed);
}

void MidiInput::collectForBlock(MidiBuffer& destination, std::uint64_t blockHostTimeNanos,
                                FrameCount frameCount, SampleRate sampleRate) noexcept
{
    destination.clear();

    if (frameCount <= 0 || sampleRate <= 0.0)
        return;

    const double nanosPerFrame = nanosPerSecond / sampleRate;
    const auto   blockNanos    = static_cast<std::uint64_t>(static_cast<double>(frameCount) * nanosPerFrame);
    const std::uint64_t blockEnd = blockHostTimeNanos + blockNanos;

    const auto place = [&](const platform::TimestampedMidiMessage& message) {
        FrameCount offset = 0;

        if (message.hostTimeNanos <= blockHostTimeNanos) {
            // Already late. Zero is the earliest moment we can still make a
            // sound at, so that is where it goes.
            late_.fetch_add(1, std::memory_order_relaxed);
        } else {
            const auto delta = static_cast<double>(message.hostTimeNanos - blockHostTimeNanos);

            // Rounded, not truncated. A nanosecond timestamp is an integer, so
            // the frame it names almost never divides exactly; truncating puts
            // every event one frame early, consistently, which is a systematic
            // timing error rather than noise. Same reasoning as
            // engine/core/Time.h secondsToFrames.
            offset = static_cast<FrameCount>(delta / nanosPerFrame + 0.5);

            if (offset >= frameCount)
                offset = frameCount - 1;
        }

        MidiMessage placed;
        placed.frameOffset = offset;
        placed.status      = message.status;
        placed.data1       = message.data1;
        placed.data2       = message.data2;

        (void)destination.insert(placed);
    };

    if (hasPending_) {
        if (pending_.hostTimeNanos >= blockEnd)
            return;   // still in the future; nothing behind it can be earlier

        place(pending_);
        hasPending_ = false;
    }

    platform::TimestampedMidiMessage message;
    while (queue_.pop(message)) {
        if (message.hostTimeNanos >= blockEnd) {
            pending_    = message;
            hasPending_ = true;
            return;
        }

        place(message);
    }
}

void MidiInput::resetCounters() noexcept
{
    received_.store(0, std::memory_order_relaxed);
    dropped_.store(0, std::memory_order_relaxed);
    late_.store(0, std::memory_order_relaxed);
}

} // namespace incdaw::engine
