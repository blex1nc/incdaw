#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::project {

/// Naming, listing and pruning of sidecar autosave packages.
///
/// Autosaves are ordinary `.incdaw` packages written next to nothing the user
/// owns: they live in their own directory (the shell passes Application
/// Support), never inside or over the user's project. The user's file is only
/// ever written by an explicit Save.
///
/// Everything here is pure filesystem policy — no timers, no AppKit — so the
/// retention behaviour is testable headlessly. The shell owns *when* to
/// autosave; this owns *where* and *what to keep*.
class Autosave {
public:
    /// How many autosaves of one project are kept before the oldest goes.
    static constexpr std::size_t defaultKeepCount = 10;

    /// "20260816-142233" — zero-padded local time, so the lexicographic order
    /// of autosave names is their chronological order.
    [[nodiscard]] static std::string stampFor(std::chrono::system_clock::time_point when);

    /// `<directory>/<projectStem>.autosave-<stamp>.incdaw`. `projectStem` is
    /// the project file's stem, or "Untitled" for a never-saved project.
    [[nodiscard]] static std::filesystem::path pathFor(const std::filesystem::path& directory,
                                                       const std::string&           projectStem,
                                                       std::chrono::system_clock::time_point when);

    /// Autosave packages for `projectStem` in `directory`, oldest first.
    /// A missing directory is an empty list, not an error.
    [[nodiscard]] static std::vector<std::filesystem::path>
    list(const std::filesystem::path& directory, const std::string& projectStem);

    /// Removes the oldest autosaves of `projectStem` beyond `keepCount`.
    /// Returns how many packages were deleted.
    static std::size_t prune(const std::filesystem::path& directory,
                             const std::string& projectStem, std::size_t keepCount);
};

} // namespace incdaw::project
