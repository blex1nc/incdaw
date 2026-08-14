#pragma once

#include "engine/midi/MidiMessage.h"

#include <array>
#include <cstddef>

namespace incdaw::engine {

/// The MIDI messages belonging to one processing block, ordered by frame offset.
///
/// Fixed capacity and no allocation: this is handed to nodes on the audio
/// thread. Capacity is generous — 1024 messages in a 128-frame block is far
/// beyond what any performance or dense automation lane produces — and overflow
/// is counted rather than silently dropped, because a metering value that says
/// "we lost events" is worth having.
template <std::size_t Capacity = 1024>
class BasicMidiBuffer {
public:
    /// Inserts keeping the buffer sorted by frame offset.
    ///
    /// Insertion sort rather than a sort pass at the end: input arrives very
    /// nearly in order already, so this is O(n) in practice, and it means the
    /// buffer is valid to read at any moment rather than only after a commit.
    bool insert(const MidiMessage& message) noexcept
    {
        if (count_ >= Capacity) {
            ++overflowed_;
            return false;
        }

        std::size_t position = count_;
        while (position > 0 && messages_[position - 1].frameOffset > message.frameOffset) {
            messages_[position] = messages_[position - 1];
            --position;
        }

        messages_[position] = message;
        ++count_;
        return true;
    }

    void clear() noexcept { count_ = 0; }

    /// Overflows since the last `resetOverflowCount`. Must be zero.
    [[nodiscard]] std::size_t overflowCount() const noexcept { return overflowed_; }
    void resetOverflowCount() noexcept { overflowed_ = 0; }

    [[nodiscard]] std::size_t size()    const noexcept { return count_; }
    [[nodiscard]] bool        isEmpty() const noexcept { return count_ == 0; }

    [[nodiscard]] const MidiMessage& operator[](std::size_t index) const noexcept { return messages_[index]; }

    [[nodiscard]] const MidiMessage* begin() const noexcept { return messages_.data(); }
    [[nodiscard]] const MidiMessage* end()   const noexcept { return messages_.data() + count_; }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

    /// Shifts every offset by `delta` and drops what falls outside
    /// [0, blockLength). Used when a block is split at a loop wrap: the second
    /// segment's events must be re-based onto the segment, not the block.
    void rebase(FrameCount delta, FrameCount blockLength) noexcept
    {
        std::size_t kept = 0;

        for (std::size_t index = 0; index < count_; ++index) {
            const FrameCount offset = messages_[index].frameOffset + delta;
            if (offset < 0 || offset >= blockLength)
                continue;

            messages_[kept] = messages_[index];
            messages_[kept].frameOffset = offset;
            ++kept;
        }

        count_ = kept;
    }

private:
    std::array<MidiMessage, Capacity> messages_{};
    std::size_t                       count_      = 0;
    std::size_t                       overflowed_ = 0;
};

using MidiBuffer = BasicMidiBuffer<1024>;

} // namespace incdaw::engine
