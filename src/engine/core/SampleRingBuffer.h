#pragma once

#include "engine/core/Time.h"

#include <atomic>
#include <cstddef>
#include <cstring>
#include <vector>

namespace incdaw::engine {

/// Single-producer, single-consumer ring of samples, sized at runtime.
///
/// The bulk cousin of LockFreeQueue, and a separate type deliberately: that
/// queue carries fixed-size messages with per-element atomics and compile-time
/// capacity, which is right for commands and wrong for audio — capture rates
/// are decided at runtime, and pushing 96,000 samples a second one atomic pair
/// at a time is pure overhead. This ring moves whole spans with two memcpys
/// and one release store per call.
///
/// The realtime rules are the producer's: `write` is wait-free and touches no
/// memory it did not already own. `reset` allocates and is for setup only.
class SampleRingBuffer {
public:
    /// Allocates for `capacity` samples, rounded up to a power of two so that
    /// wrapping is a mask. Never call on the audio thread.
    void reset(std::size_t capacity)
    {
        std::size_t rounded = 2;
        while (rounded < capacity + 1)   // +1: one slot distinguishes full from empty
            rounded <<= 1;

        storage_.assign(rounded, 0.0f);
        mask_ = rounded - 1;
        writeIndex_.store(0, std::memory_order_release);
        readIndex_.store(0, std::memory_order_release);
    }

    /// Producer side. Copies as many samples as fit and returns how many —
    /// the caller decides whether a short write is a dropped block or fatal.
    [[nodiscard]] std::size_t write(const Sample* data, std::size_t count) noexcept
    {
        const std::size_t write = writeIndex_.load(std::memory_order_relaxed);
        const std::size_t read  = readIndex_.load(std::memory_order_acquire);

        const std::size_t free = mask_ - ((write - read) & mask_);
        const std::size_t todo = count < free ? count : free;

        // At most two segments: up to the end of the buffer, then the wrap.
        const std::size_t offset = write & mask_;
        const std::size_t first  = todo < storage_.size() - offset ? todo : storage_.size() - offset;

        std::memcpy(storage_.data() + offset, data, first * sizeof(Sample));
        std::memcpy(storage_.data(), data + first, (todo - first) * sizeof(Sample));

        writeIndex_.store(write + todo, std::memory_order_release);
        return todo;
    }

    /// Consumer side. Copies up to `count` samples out and returns how many.
    [[nodiscard]] std::size_t read(Sample* out, std::size_t count) noexcept
    {
        const std::size_t read  = readIndex_.load(std::memory_order_relaxed);
        const std::size_t write = writeIndex_.load(std::memory_order_acquire);

        const std::size_t available = (write - read) & mask_;
        const std::size_t todo      = count < available ? count : available;

        const std::size_t offset = read & mask_;
        const std::size_t first  = todo < storage_.size() - offset ? todo : storage_.size() - offset;

        std::memcpy(out, storage_.data() + offset, first * sizeof(Sample));
        std::memcpy(out + first, storage_.data(), (todo - first) * sizeof(Sample));

        readIndex_.store(read + todo, std::memory_order_release);
        return todo;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return (writeIndex_.load(std::memory_order_acquire)
                - readIndex_.load(std::memory_order_acquire)) & mask_;
    }

    [[nodiscard]] std::size_t freeSpace() const noexcept { return capacity() - size(); }
    [[nodiscard]] std::size_t capacity()  const noexcept { return mask_; }

private:
    static constexpr std::size_t cacheLineSize = 64;   // see LockFreeQueue.h

    std::vector<Sample> storage_;
    std::size_t         mask_ = 0;

    // Indices grow monotonically and are masked on use; keeping them unwrapped
    // makes size() a subtraction with no full/empty ambiguity.
    alignas(cacheLineSize) std::atomic<std::size_t> writeIndex_{0};
    alignas(cacheLineSize) std::atomic<std::size_t> readIndex_{0};
};

} // namespace incdaw::engine
