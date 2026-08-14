#include "platform/SystemInfo.h"

#include "platform/Platform.h"

#if INCDAW_PLATFORM_MACOS
    #include <sys/sysctl.h>
    #include <unistd.h>
#endif

namespace incdaw::platform {
namespace {

#if INCDAW_PLATFORM_MACOS

std::size_t sysctlUInt(const char* name) noexcept
{
    std::uint64_t value = 0;
    std::size_t   size  = sizeof(value);

    if (::sysctlbyname(name, &value, &size, nullptr, 0) != 0)
        return 0;

    // Some keys report 32-bit values; `size` tells us which we got.
    if (size == sizeof(std::uint32_t))
        return static_cast<std::size_t>(*reinterpret_cast<std::uint32_t*>(&value));

    return static_cast<std::size_t>(value);
}

std::string sysctlString(const char* name)
{
    std::size_t size = 0;
    if (::sysctlbyname(name, nullptr, &size, nullptr, 0) != 0 || size == 0)
        return {};

    std::string value(size, '\0');
    if (::sysctlbyname(name, value.data(), &size, nullptr, 0) != 0)
        return {};

    // sysctl includes the terminating NUL in `size`.
    if (!value.empty() && value.back() == '\0')
        value.pop_back();

    return value;
}

#endif // INCDAW_PLATFORM_MACOS

} // namespace

SystemInfo SystemInfo::query()
{
    SystemInfo info;

#if INCDAW_PLATFORM_MACOS
    info.cpuBrand            = sysctlString("machdep.cpu.brand_string");
    info.logicalCoreCount    = sysctlUInt("hw.logicalcpu");
    info.physicalMemoryBytes = sysctlUInt("hw.memsize");
    info.pageSizeBytes       = sysctlUInt("hw.pagesize");

    // Apple silicon exposes per-cluster counts. Intel Macs do not, in which
    // case every core is treated as a performance core.
    info.performanceCoreCount = sysctlUInt("hw.perflevel0.logicalcpu");
    info.efficiencyCoreCount  = sysctlUInt("hw.perflevel1.logicalcpu");

    if (info.performanceCoreCount == 0)
        info.performanceCoreCount = info.logicalCoreCount;
#endif

    return info;
}

std::size_t SystemInfo::suggestedRealtimeWorkerCount() const noexcept
{
    // Reserve one performance core for the device callback thread. Efficiency
    // cores are deliberately excluded: work scheduled there arrives late and
    // stalls the whole block.
    return performanceCoreCount > 1 ? performanceCoreCount - 1 : 0;
}

} // namespace incdaw::platform
