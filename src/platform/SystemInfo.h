#pragma once

#include <cstddef>
#include <string>

namespace incdaw::platform {

/// Static facts about the machine INCDAW is running on.
///
/// Queried once at startup. The audio engine uses `performanceCoreCount` to
/// size its worker pool: on heterogeneous CPUs (Apple silicon) spawning one
/// realtime worker per *logical* core oversubscribes the performance cores and
/// causes underruns.
struct SystemInfo {
    std::string cpuBrand;
    std::size_t logicalCoreCount     = 0;
    std::size_t performanceCoreCount = 0;
    std::size_t efficiencyCoreCount  = 0;
    std::size_t physicalMemoryBytes  = 0;
    std::size_t pageSizeBytes        = 0;

    /// Queries the operating system. Not realtime-safe; call once at startup.
    static SystemInfo query();

    /// Worker threads the realtime pool should use, excluding the device
    /// callback thread itself. Zero means "render everything on the callback
    /// thread", which is correct on a single-performance-core machine.
    [[nodiscard]] std::size_t suggestedRealtimeWorkerCount() const noexcept;
};

} // namespace incdaw::platform
