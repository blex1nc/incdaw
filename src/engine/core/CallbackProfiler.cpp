#include "engine/core/CallbackProfiler.h"

namespace incdaw::engine {

double CallbackProfiler::loadPercentile(double fraction) const noexcept
{
    const std::uint64_t total = callbackCount_.load(std::memory_order_relaxed);
    if (total == 0)
        return 0.0;

    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;

    const auto target = static_cast<std::uint64_t>(static_cast<double>(total) * fraction);

    std::uint64_t seen = 0;
    for (std::size_t index = 0; index < bucketCount; ++index) {
        seen += buckets_[index].load(std::memory_order_relaxed);
        if (seen >= target)
            return static_cast<double>(index + 1) * bucketWidth;
    }

    return static_cast<double>(bucketCount) * bucketWidth;
}

void CallbackProfiler::reset() noexcept
{
    for (auto& bucket : buckets_)
        bucket.store(0, std::memory_order_relaxed);

    callbackCount_.store(0, std::memory_order_relaxed);
    underruns_.store(0, std::memory_order_relaxed);
    peakLoad_.store(0.0, std::memory_order_relaxed);
}

} // namespace incdaw::engine
