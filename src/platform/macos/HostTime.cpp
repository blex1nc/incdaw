#include "platform/HostTime.h"

#include "platform/Platform.h"

#if INCDAW_PLATFORM_MACOS

#include <mach/mach_time.h>

namespace incdaw::platform {
namespace {

struct Timebase {
    Timebase() noexcept
    {
        mach_timebase_info_data_t info{};
        if (mach_timebase_info(&info) == KERN_SUCCESS && info.denom != 0) {
            numerator   = info.numer;
            denominator = info.denom;
        }
    }

    std::uint64_t numerator   = 1;
    std::uint64_t denominator = 1;
};

/// Queried once. `mach_timebase_info` is a syscall on first use, and the audio
/// thread must not be the one to make it.
const Timebase& timebase() noexcept
{
    static const Timebase instance;
    return instance;
}

} // namespace

std::uint64_t hostTimeToNanos(std::uint64_t hostTime) noexcept
{
    const Timebase& base = timebase();

    // On Apple silicon the ratio is not 1:1, so the multiply cannot be skipped.
    // Dividing last keeps the full precision of a 64-bit tick count for the
    // ~292 years before it would overflow.
    if (base.numerator == base.denominator)
        return hostTime;

    const std::uint64_t whole     = hostTime / base.denominator;
    const std::uint64_t remainder = hostTime % base.denominator;

    return whole * base.numerator + remainder * base.numerator / base.denominator;
}

std::uint64_t nanosToHostTime(std::uint64_t nanos) noexcept
{
    const Timebase& base = timebase();

    if (base.numerator == base.denominator)
        return nanos;

    // Mirror of hostTimeToNanos with the ratio inverted, split the same way so
    // that a large tick count keeps its precision rather than overflowing the
    // multiply.
    const std::uint64_t whole     = nanos / base.numerator;
    const std::uint64_t remainder = nanos % base.numerator;

    return whole * base.denominator + remainder * base.denominator / base.numerator;
}

std::uint64_t hostTimeNowNanos() noexcept
{
    return hostTimeToNanos(mach_absolute_time());
}

} // namespace incdaw::platform

#endif // INCDAW_PLATFORM_MACOS
