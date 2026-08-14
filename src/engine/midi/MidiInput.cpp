#include "engine/midi/MidiInput.h"

namespace incdaw::engine {
namespace {

constexpr double nanosPerSecond = 1'000'000'000.0;

} // namespace

void MidiInput::midiMessageReceived(const platform::TimestampedMidiMessage& message) noexcept
{
    received_.fetch_add(1, std::memory_order_relaxed);

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
