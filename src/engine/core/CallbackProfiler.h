#pragma once

#include "engine/core/Time.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace incdaw::engine {

/// Measures how long the audio callback takes, as a distribution.
///
/// docs/PERFORMANCE.md §2: headroom matters more than the average. A callback
/// averaging 40% of its budget but spiking to 110% glitches audibly, while one
/// sitting flat at 60% does not — and an average hides exactly that difference.
/// So this records a histogram and a maximum, not a mean.
///
/// Recording is realtime-safe: two relaxed atomic increments and no branching
/// on shared state. Reading is done from the UI thread and may be slightly
/// stale, which is fine for a meter.
class CallbackProfiler {
public:
    /// Buckets are fractions of the callback budget, in 5% steps up to 200%.
    /// Anything at or above 100% is an underrun in progress.
    static constexpr std::size_t bucketCount     = 40;
    static constexpr double      bucketWidth     = 0.05;
    static constexpr std::size_t overrunBucket   = 20;   // the bucket at 100%

    void configure(SampleRate sampleRate, FrameCount blockSize) noexcept
    {
        budgetSeconds_.store(sampleRate > 0.0 ? static_cast<double>(blockSize) / sampleRate : 0.0,
                             std::memory_order_relaxed);
        reset();
    }

    /// Realtime-safe. `elapsedSeconds` is the measured callback duration and
    /// `frameCount` is how many frames this callback actually rendered.
    ///
    /// The budget is computed per block rather than taken from the configured
    /// buffer size, because the two can differ: when a device is shared with
    /// another process, CoreAudio delivers whatever block size it is servicing,
    /// not the size our property query reported. Dividing by the nominal figure
    /// then understates the load — by 2x in the case that surfaced this.
    void record(double elapsedSeconds, FrameCount frameCount, SampleRate sampleRate) noexcept
    {
        const double budget = sampleRate > 0.0 && frameCount > 0
                                  ? static_cast<double>(frameCount) / sampleRate
                                  : budgetSeconds_.load(std::memory_order_relaxed);

        if (budget <= 0.0)
            return;

        const double load = elapsedSeconds / budget;

        auto bucket = static_cast<std::size_t>(load / bucketWidth);
        if (bucket >= bucketCount)
            bucket = bucketCount - 1;

        buckets_[bucket].fetch_add(1, std::memory_order_relaxed);
        callbackCount_.fetch_add(1, std::memory_order_relaxed);

        // Monotonic maximum via compare-exchange. The audio thread is the only
        // writer, so this never spins more than once in practice.
        double previousPeak = peakLoad_.load(std::memory_order_relaxed);
        while (load > previousPeak
               && !peakLoad_.compare_exchange_weak(previousPeak, load, std::memory_order_relaxed))
            ;
    }

    void recordUnderrun() noexcept { underruns_.fetch_add(1, std::memory_order_relaxed); }

    [[nodiscard]] std::uint64_t callbackCount() const noexcept { return callbackCount_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t underrunCount() const noexcept { return underruns_.load(std::memory_order_relaxed); }
    [[nodiscard]] double        peakLoad()      const noexcept { return peakLoad_.load(std::memory_order_relaxed); }
    [[nodiscard]] double        budgetSeconds() const noexcept { return budgetSeconds_.load(std::memory_order_relaxed); }

    [[nodiscard]] std::uint64_t bucket(std::size_t index) const noexcept
    {
        return index < bucketCount ? buckets_[index].load(std::memory_order_relaxed) : 0;
    }

    /// Callbacks that took at least their whole budget. Every one of these is a
    /// dropout the user can hear.
    [[nodiscard]] std::uint64_t overrunCount() const noexcept
    {
        std::uint64_t total = 0;
        for (std::size_t index = overrunBucket; index < bucketCount; ++index)
            total += buckets_[index].load(std::memory_order_relaxed);
        return total;
    }

    /// Load below which the given fraction of callbacks completed, e.g.
    /// `loadPercentile(0.99)`. Resolution is one bucket (5%).
    [[nodiscard]] double loadPercentile(double fraction) const noexcept;

    void reset() noexcept;

private:
    std::array<std::atomic<std::uint64_t>, bucketCount> buckets_{};
    std::atomic<std::uint64_t> callbackCount_{0};
    std::atomic<std::uint64_t> underruns_{0};
    std::atomic<double>        peakLoad_{0.0};
    std::atomic<double>        budgetSeconds_{0.0};
};

} // namespace incdaw::engine
