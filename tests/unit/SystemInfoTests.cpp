#include "doctest.h"

#include "platform/SystemInfo.h"

using namespace incdaw::platform;

TEST_CASE("the machine reports a usable description of itself")
{
    const auto info = SystemInfo::query();

    CHECK(info.logicalCoreCount > 0);
    CHECK(info.physicalMemoryBytes > 0);
    CHECK(info.pageSizeBytes > 0);
    CHECK_FALSE(info.cpuBrand.empty());
}

TEST_CASE("core counts are internally consistent")
{
    const auto info = SystemInfo::query();

    CHECK(info.performanceCoreCount > 0);
    CHECK(info.performanceCoreCount <= info.logicalCoreCount);
    CHECK(info.efficiencyCoreCount <= info.logicalCoreCount);

    // On heterogeneous CPUs the clusters should account for every logical core.
    if (info.efficiencyCoreCount > 0)
        CHECK(info.performanceCoreCount + info.efficiencyCoreCount == info.logicalCoreCount);
}

TEST_CASE("the realtime worker count reserves a core for the device callback")
{
    const auto info = SystemInfo::query();
    const auto workers = info.suggestedRealtimeWorkerCount();

    CHECK(workers < info.performanceCoreCount);

    // Efficiency cores must never be counted: work scheduled there arrives late
    // and stalls the entire audio block.
    CHECK(workers <= info.performanceCoreCount - 1);
}
