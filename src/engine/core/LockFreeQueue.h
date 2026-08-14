#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>

namespace incdaw::engine {

/// Single-producer, single-consumer lock-free ring buffer.
///
/// The only sanctioned way for the UI thread and the audio thread to talk
/// (docs/ARCHITECTURE.md §4). A mutex shared with the audio thread invites
/// priority inversion: the audio thread, at realtime priority, blocks on a lock
/// held by a normal-priority thread that the scheduler has no reason to run.
/// The result is a dropout, and it happens under load, which is exactly when it
/// is hardest to reproduce.
///
/// Capacity is fixed at compile time and rounded to a power of two so that
/// index wrapping is a mask rather than a division.
///
/// `T` must be trivially copyable: messages crossing this boundary carry no
/// ownership, because destroying an object on the audio thread would free
/// memory (docs/AUDIO_ENGINE.md §4). Objects the audio thread finishes with go
/// to the reaper instead.
template <typename T, std::size_t Capacity>
class LockFreeQueue {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Messages crossing the realtime boundary must be trivially copyable: "
                  "the audio thread must never run a destructor that frees memory.");
    static_assert(Capacity >= 2, "Capacity must be at least 2.");
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two.");

public:
    LockFreeQueue() = default;

    LockFreeQueue(const LockFreeQueue&)            = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;

    /// Producer side. Returns false if the queue is full — the caller decides
    /// whether that is fatal or simply a dropped meter update.
    [[nodiscard]] bool push(const T& value) noexcept
    {
        const std::size_t write = writeIndex_.load(std::memory_order_relaxed);
        const std::size_t next  = (write + 1) & mask;

        // Acquire: we must see the consumer's read index before deciding there
        // is room, or we could overwrite an unread slot.
        if (next == readIndex_.load(std::memory_order_acquire))
            return false;

        storage_[write] = value;

        // Release: the value must be visible before the index that publishes it.
        writeIndex_.store(next, std::memory_order_release);
        return true;
    }

    /// Consumer side. Returns false if the queue is empty.
    [[nodiscard]] bool pop(T& out) noexcept
    {
        const std::size_t read = readIndex_.load(std::memory_order_relaxed);

        if (read == writeIndex_.load(std::memory_order_acquire))
            return false;

        out = storage_[read];
        readIndex_.store((read + 1) & mask, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool isEmpty() const noexcept
    {
        return readIndex_.load(std::memory_order_acquire)
            == writeIndex_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        const std::size_t write = writeIndex_.load(std::memory_order_acquire);
        const std::size_t read  = readIndex_.load(std::memory_order_acquire);
        return (write - read) & mask;
    }

    /// One slot is always kept empty to distinguish "full" from "empty".
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity - 1; }

private:
    static constexpr std::size_t mask = Capacity - 1;

    // Hardware destructive interference size, hardcoded rather than taken from
    // std::hardware_destructive_interference_size: that constant is an ABI
    // hazard under -Winterference-size and is 64 on every CPU INCDAW targets.
    static constexpr std::size_t cacheLineSize = 64;

    std::array<T, Capacity> storage_{};

    // The two indices must not share a cache line. If they do, every producer
    // write invalidates the consumer's line and vice versa — false sharing that
    // costs far more than the queue itself.
    alignas(cacheLineSize) std::atomic<std::size_t> writeIndex_{0};
    alignas(cacheLineSize) std::atomic<std::size_t> readIndex_{0};
};

} // namespace incdaw::engine
