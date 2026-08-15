#pragma once

#include "engine/graph/Node.h"
#include "plugins/PluginIdentifier.h"
#include "plugins/PluginParameterInfo.h"
#include "plugins/PluginRegistry.h"
#include "plugins/clap/ClapLibrary.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace incdaw::plugins {

/// Owns the loaded plugin libraries for the application's lifetime.
///
/// A PluginNode owns its instance, but destroying that instance calls back
/// into the library it came from — so the library has to outlive every node
/// made from it. Graphs are rebuilt on every edit and retired asynchronously
/// by the reaper, which means a library owned by a graph would be unloaded
/// while a node from it was still queued for destruction. That is a
/// use-after-free with a plugin's code on the far side of it.
///
/// Libraries therefore live here, keyed by path, and are never closed while
/// the manager exists. Loading is also expensive enough that reloading a
/// binary per keystroke would be its own reason not to.
///
/// Non-realtime only: this allocates, opens files and runs plugin code. The
/// mutex exists because compilation may move off the UI thread later, not
/// because it is contended today.
class PluginInstanceManager {
public:
    explicit PluginInstanceManager(const PluginRegistry& registry) noexcept
        : registry_(&registry) {}

    PluginInstanceManager(const PluginInstanceManager&)            = delete;
    PluginInstanceManager& operator=(const PluginInstanceManager&) = delete;

    /// Instantiates `identifier` and wraps it in a render-graph node.
    ///
    /// Returns nullptr with a reason rather than throwing or failing the
    /// compile: an insert that cannot be created is a pass-through slot the
    /// UI should explain, not a project that refuses to play.
    [[nodiscard]] std::unique_ptr<engine::Node> createInsert(const PluginIdentifier& identifier,
                                                             double                  sampleRate,
                                                             std::uint32_t           maxFrames,
                                                             std::string&            error);

    /// Parameters of `uid` as discovered by its first instantiation, or
    /// nullptr before one has succeeded. Discovery is per plugin TYPE —
    /// instances of one plugin share the list ("discovered once and cached",
    /// docs/PLUGIN_HOST.md §5). The pointer stays valid for the manager's
    /// lifetime: entries are never erased.
    [[nodiscard]] const std::vector<PluginParameterInfo>* parametersFor(
        const std::string& uid) const;

    /// Libraries opened so far. The caching assertion in the tests.
    [[nodiscard]] std::size_t loadedLibraryCount() const;

private:
    /// Caller holds `mutex_`.
    [[nodiscard]] ClapLibrary* libraryFor(const std::string& path, std::string& error);

    const PluginRegistry* registry_ = nullptr;

    std::unordered_map<std::string, std::unique_ptr<ClapLibrary>>     libraries_;
    std::unordered_map<std::string, std::vector<PluginParameterInfo>> parameters_;
    mutable std::mutex                                                mutex_;
};

} // namespace incdaw::plugins
