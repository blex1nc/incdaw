#pragma once

#include <string>
#include <vector>

namespace incdaw::app {

/// The Open Recent list as pure policy: most recent first, no duplicates,
/// capped. The shell owns persistence (NSUserDefaults) and the menu; keeping
/// the list rules here keeps them testable without AppKit.
class RecentProjects {
public:
    static constexpr std::size_t maximumCount = 10;

    /// `list` with `path` moved (or inserted) at the front, capped. Paths are
    /// compared exactly: normalisation is the caller's business, since only
    /// the shell knows the platform's path semantics.
    [[nodiscard]] static std::vector<std::string> updated(std::vector<std::string> list,
                                                          const std::string&       path);

    /// `list` without `path` — for entries that turned out not to exist.
    [[nodiscard]] static std::vector<std::string> without(std::vector<std::string> list,
                                                          const std::string&       path);
};

} // namespace incdaw::app
