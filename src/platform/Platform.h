#pragma once

// Compile-time platform identification.
//
// This header exists so that the rest of the codebase never needs to test for
// an operating system: only platform/ may branch on these macros. See
// docs/ARCHITECTURE.md §2.

#if defined(__APPLE__)
    #define INCDAW_PLATFORM_MACOS 1
#else
    #define INCDAW_PLATFORM_MACOS 0
#endif

#if defined(_WIN32)
    #define INCDAW_PLATFORM_WINDOWS 1
#else
    #define INCDAW_PLATFORM_WINDOWS 0
#endif

#if !INCDAW_PLATFORM_MACOS && !INCDAW_PLATFORM_WINDOWS
    #error "INCDAW currently supports macOS. Windows is a planned target (see docs/DECISIONS.md D-005)."
#endif
