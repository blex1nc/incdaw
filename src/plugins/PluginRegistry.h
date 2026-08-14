#pragma once

#include "plugins/PluginScan.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::plugins {

/// The persisted plugin catalogue (docs/PLUGIN_HOST.md §3).
///
/// Startup loads this file and touches no plugin binary at all — startup
/// time must not scale with the size of a plugin collection. Scanning goes
/// through the out-of-process scanner, one library per child process, and a
/// library that crashed or failed lands on the blacklist WITH its reason:
/// it is skipped on every later scan until the user clears it, so one
/// broken plugin can never make every launch slow or dangerous again.
///
/// The cache key is (size, mtime): an updated binary rescans, an untouched
/// one never spawns a process.
class PluginRegistry {
public:
    struct Library {
        std::string   path;
        std::uint64_t fileSize = 0;
        std::int64_t  mtimeSeconds = 0;

        bool        blacklisted = false;
        std::string blacklistReason;

        std::vector<ClapDescriptor> plugins;   ///< empty when blacklisted
    };

    /// Scans every .clap in `directory` (recursively), reusing cached
    /// entries whose size and mtime are unchanged. Returns how many child
    /// scans actually ran — what the caching tests assert on.
    std::size_t scanDirectory(const std::filesystem::path& directory,
                              const std::filesystem::path& scannerBinary);

    [[nodiscard]] const std::vector<Library>& libraries() const noexcept { return libraries_; }

    /// Every known, non-blacklisted plugin with the library it lives in.
    struct Located {
        const Library*        library = nullptr;
        const ClapDescriptor* plugin  = nullptr;
    };
    [[nodiscard]] std::vector<Located> plugins() const;
    [[nodiscard]] Located find(const std::string& pluginId) const;

    /// Forgets every blacklist entry, so the next scan retries them. The
    /// user asked for another chance; the registry does not argue.
    void clearBlacklist();

    /// Versioned TSV. Loading replaces the current contents.
    [[nodiscard]] bool save(const std::filesystem::path& path) const;
    [[nodiscard]] bool load(const std::filesystem::path& path);

private:
    std::vector<Library> libraries_;
};

} // namespace incdaw::plugins
