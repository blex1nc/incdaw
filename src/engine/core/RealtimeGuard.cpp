#include "engine/core/RealtimeGuard.h"

#include <atomic>
#include <cstdlib>
#include <new>

namespace incdaw::engine::rt {
namespace {

// Depth rather than a bool so that nested guarded scopes behave correctly.
thread_local int  realtimeDepth   = 0;
thread_local bool exemptionActive = false;

std::atomic<std::size_t> allocationCount{0};
std::atomic<std::size_t> deallocationCount{0};

} // namespace

bool isInsideRealtimeContext() noexcept
{
    return realtimeDepth > 0 && !exemptionActive;
}

std::size_t allocationViolations() noexcept
{
    return allocationCount.load(std::memory_order_relaxed);
}

std::size_t deallocationViolations() noexcept
{
    return deallocationCount.load(std::memory_order_relaxed);
}

void resetViolations() noexcept
{
    allocationCount.store(0, std::memory_order_relaxed);
    deallocationCount.store(0, std::memory_order_relaxed);
}

ScopedRealtimeContext::ScopedRealtimeContext() noexcept { ++realtimeDepth; }
ScopedRealtimeContext::~ScopedRealtimeContext() noexcept { --realtimeDepth; }

ScopedRealtimeExemption::ScopedRealtimeExemption(const char* reason) noexcept
    : wasActive_(exemptionActive)
{
    (void)reason;   // retained for the reader; every use must justify itself
    exemptionActive = true;
}

ScopedRealtimeExemption::~ScopedRealtimeExemption() noexcept
{
    exemptionActive = wasActive_;
}

namespace detail {

void recordAllocation() noexcept
{
    if (isInsideRealtimeContext())
        allocationCount.fetch_add(1, std::memory_order_relaxed);
}

void recordDeallocation() noexcept
{
    if (isInsideRealtimeContext())
        deallocationCount.fetch_add(1, std::memory_order_relaxed);
}

} // namespace detail
} // namespace incdaw::engine::rt

#if INCDAW_REALTIME_GUARD

// Global allocator interception.
//
// These replace the implementations for the whole program, which is exactly the
// point: an allocation anywhere beneath the audio callback — including inside a
// third-party library or a container we did not expect to grow — is caught.
//
// std::malloc/std::free are used rather than delegating to the default
// operators, because delegating would recurse.

void* operator new(std::size_t size)
{
    incdaw::engine::rt::detail::recordAllocation();

    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;

    throw std::bad_alloc{};
}

void* operator new[](std::size_t size) { return ::operator new(size); }

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    incdaw::engine::rt::detail::recordAllocation();
    return std::malloc(size == 0 ? 1 : size);
}

void* operator new[](std::size_t size, const std::nothrow_t& tag) noexcept
{
    return ::operator new(size, tag);
}

void operator delete(void* memory) noexcept
{
    incdaw::engine::rt::detail::recordDeallocation();
    std::free(memory);
}

void operator delete[](void* memory) noexcept { ::operator delete(memory); }

void operator delete(void* memory, std::size_t) noexcept { ::operator delete(memory); }
void operator delete[](void* memory, std::size_t) noexcept { ::operator delete(memory); }

void operator delete(void* memory, const std::nothrow_t&) noexcept { ::operator delete(memory); }
void operator delete[](void* memory, const std::nothrow_t&) noexcept { ::operator delete(memory); }

// Aligned overloads: std::aligned_storage and over-aligned SIMD types route
// here, and missing them would let allocations slip past the guard.
void* operator new(std::size_t size, std::align_val_t alignment)
{
    incdaw::engine::rt::detail::recordAllocation();

    // macOS requires the size passed to aligned_alloc to be an integral
    // multiple of the alignment; rounding up here rather than relying on a
    // platform that happens to be lenient.
    const std::size_t align   = static_cast<std::size_t>(alignment);
    const std::size_t rounded = ((size == 0 ? 1 : size) + align - 1) / align * align;

    if (void* memory = std::aligned_alloc(align, rounded))
        return memory;

    throw std::bad_alloc{};
}

void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return ::operator new(size, alignment);
}

void operator delete(void* memory, std::align_val_t) noexcept
{
    incdaw::engine::rt::detail::recordDeallocation();
    std::free(memory);
}

void operator delete[](void* memory, std::align_val_t alignment) noexcept
{
    ::operator delete(memory, alignment);
}

void operator delete(void* memory, std::size_t, std::align_val_t alignment) noexcept
{
    ::operator delete(memory, alignment);
}

void operator delete[](void* memory, std::size_t, std::align_val_t alignment) noexcept
{
    ::operator delete(memory, alignment);
}

#endif // INCDAW_REALTIME_GUARD
