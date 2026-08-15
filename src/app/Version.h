#pragma once

namespace incdaw::app {

struct Version {
    static constexpr int major = 0;
    static constexpr int minor = 9;
    static constexpr int patch = 0;

    /// e.g. "0.1.0"
    [[nodiscard]] static const char* string() noexcept;

    /// Development phase reached, per docs/ROADMAP.md. Displayed in the UI so
    /// that a build never implies more capability than it has.
    [[nodiscard]] static const char* phase() noexcept;
};

} // namespace incdaw::app
