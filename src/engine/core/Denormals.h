#pragma once

#include <cstdint>

#if defined(__aarch64__) || defined(_M_ARM64)
    #define INCDAW_ARM64 1
#else
    #define INCDAW_ARM64 0
#endif

namespace incdaw::engine {

/// Disables denormal floating-point numbers for the enclosing scope.
///
/// A denormal is a number so small it falls outside the normal exponent range.
/// Arithmetic on them is handled by a slow path that can cost tens of times a
/// normal operation. Every decaying tail in a reverb or filter eventually
/// reaches that range, so an untreated DAW shows sudden CPU spikes seconds
/// after a sound has become inaudible — the classic "it glitches when the music
/// stops" bug.
///
/// Flushing them to zero is inaudible: the values are below the noise floor by
/// a wide margin.
///
/// Constructed at the top of every audio callback, before any DSP runs.
class ScopedNoDenormals {
public:
    ScopedNoDenormals() noexcept : previous_(readControlRegister())
    {
        // FZ (bit 24) flushes denormal results to zero. On arm64 this covers
        // both inputs and outputs; x86 needs FTZ and DAZ set separately.
        writeControlRegister(previous_ | flushToZeroMask);
    }

    ~ScopedNoDenormals() noexcept { writeControlRegister(previous_); }

    ScopedNoDenormals(const ScopedNoDenormals&)            = delete;
    ScopedNoDenormals& operator=(const ScopedNoDenormals&) = delete;

    /// True if denormals are currently flushed. Used by tests to prove the
    /// scope actually took effect rather than silently doing nothing.
    [[nodiscard]] static bool areDenormalsDisabled() noexcept
    {
        return (readControlRegister() & flushToZeroMask) != 0;
    }

private:
#if INCDAW_ARM64
    using ControlRegister = std::uint64_t;
    static constexpr ControlRegister flushToZeroMask = 1ull << 24;   // FPCR.FZ

    [[nodiscard]] static ControlRegister readControlRegister() noexcept
    {
        ControlRegister value = 0;
        __asm__ __volatile__("mrs %0, fpcr" : "=r"(value));
        return value;
    }

    static void writeControlRegister(ControlRegister value) noexcept
    {
        __asm__ __volatile__("msr fpcr, %0" : : "r"(value));
    }
#else
    using ControlRegister = std::uint32_t;
    static constexpr ControlRegister flushToZeroMask = 0x8040u;      // MXCSR FTZ | DAZ

    [[nodiscard]] static ControlRegister readControlRegister() noexcept
    {
        ControlRegister value = 0;
        __asm__ __volatile__("stmxcsr %0" : "=m"(value));
        return value;
    }

    static void writeControlRegister(ControlRegister value) noexcept
    {
        __asm__ __volatile__("ldmxcsr %0" : : "m"(value));
    }
#endif

    ControlRegister previous_;
};

} // namespace incdaw::engine
