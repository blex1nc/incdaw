#pragma once

#include <cstddef>

/// Realtime-safety enforcement.
///
/// docs/AUDIO_ENGINE.md §1 states the prime directive: the audio thread never
/// allocates, never locks, never performs I/O. Stating it is not enough — this
/// header makes it mechanically checkable.
///
/// In guarded builds, entering the audio callback marks the thread. A global
/// `operator new`/`delete` override then records any allocation that happens
/// while the mark is set. Tests assert the count is zero; a violation fails the
/// suite on the commit that introduced it.
///
/// The guard deliberately RECORDS rather than aborts. Calling `abort()` from
/// inside a device callback produces a crash whose stack is often useless,
/// whereas a counter plus a test assertion points at the exact commit.
namespace incdaw::engine::rt {

/// True when the calling thread is currently inside an audio callback.
[[nodiscard]] bool isInsideRealtimeContext() noexcept;

/// Number of heap allocations observed inside a realtime context since the last
/// reset. Must be zero.
[[nodiscard]] std::size_t allocationViolations() noexcept;

/// Number of heap deallocations observed inside a realtime context. Also must
/// be zero: `free` takes the allocator lock exactly as `malloc` does.
[[nodiscard]] std::size_t deallocationViolations() noexcept;

void resetViolations() noexcept;

/// True if this build actually enforces the rules. False in unguarded builds,
/// so tests can report "not verified" instead of silently passing.
[[nodiscard]] constexpr bool guardEnabled() noexcept
{
#if INCDAW_REALTIME_GUARD
    return true;
#else
    return false;
#endif
}

/// Marks the enclosing scope as realtime. Constructed at the top of every audio
/// callback.
///
/// Nesting is supported (a callback may call into another guarded region)
/// because the mark is a counter, not a flag.
class ScopedRealtimeContext {
public:
    ScopedRealtimeContext() noexcept;
    ~ScopedRealtimeContext() noexcept;

    ScopedRealtimeContext(const ScopedRealtimeContext&)            = delete;
    ScopedRealtimeContext& operator=(const ScopedRealtimeContext&) = delete;
};

/// Temporarily lifts the mark.
///
/// Needed only where a realtime thread must legitimately call code that
/// allocates — currently nowhere. It exists so that any such case has to be
/// written down explicitly and reviewed, rather than silently slipping past.
class ScopedRealtimeExemption {
public:
    explicit ScopedRealtimeExemption(const char* reason) noexcept;
    ~ScopedRealtimeExemption() noexcept;

    ScopedRealtimeExemption(const ScopedRealtimeExemption&)            = delete;
    ScopedRealtimeExemption& operator=(const ScopedRealtimeExemption&) = delete;

private:
    bool wasActive_ = false;
};

} // namespace incdaw::engine::rt
