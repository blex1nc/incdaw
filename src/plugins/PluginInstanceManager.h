#pragma once

#include "engine/graph/Node.h"
#include "plugins/PluginIdentifier.h"
#include "plugins/PluginParameterInfo.h"
#include "plugins/PluginRegistry.h"
#include "plugins/HostedPlugin.h"
#include "plugins/clap/ClapLibrary.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace incdaw::plugins {

/// Owns the loaded plugin libraries AND the live plugin instances.
///
/// Libraries live for the application's lifetime, keyed by path: destroying
/// an instance calls back into the library it came from, graphs are retired
/// asynchronously by the reaper, and loading a binary is expensive.
///
/// Instances live for their SLOT's lifetime, keyed by the slot's entity id
/// (D-031). Graphs are rebuilt on every edit; an instance that died with its
/// node would lose its live state — and dangle under any open editor — every
/// time the user added a note. The graph node therefore BORROWS the instance,
/// and only `retainOnlyInstances` (called after a rebuild, with the slots the
/// project still contains) actually destroys one. Two graphs may briefly both
/// name the instance, but only one graph is ever processed: the engine swaps
/// atomically between blocks, so the audio thread never runs it twice.
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

    /// Returns a render-graph node for `slotKey`, reusing the slot's live
    /// instance when the plugin, sample rate and block size still match.
    /// A changed rate or block size recreates the instance and carries its
    /// state blob across, so the plugin does not audibly reset.
    ///
    /// Returns nullptr with a reason rather than throwing or failing the
    /// compile: an insert that cannot be created is a pass-through slot the
    /// UI should explain, not a project that refuses to play.
    [[nodiscard]] std::unique_ptr<engine::Node> createInsert(std::uint64_t           slotKey,
                                                             const PluginIdentifier& identifier,
                                                             double                  sampleRate,
                                                             std::uint32_t           maxFrames,
                                                             std::string&            error);

    /// Destroys every instance whose slot key is not in `slotKeys` — the
    /// slots the project still contains, bypassed ones included (a bypassed
    /// slot keeps its instance and therefore its live state). Main thread,
    /// after the engine has swapped to the rebuilt graph.
    void retainOnlyInstances(const std::vector<std::uint64_t>& slotKeys);

    /// The live instance for a slot, or nullptr. Borrowed: valid until the
    /// slot leaves the project and a retain pass runs.
    [[nodiscard]] HostedPlugin* instanceFor(std::uint64_t slotKey) const;

    /// Parameters of `uid` as discovered by its first instantiation, or
    /// nullptr before one has succeeded. Discovery is per plugin TYPE —
    /// instances of one plugin share the list ("discovered once and cached",
    /// docs/PLUGIN_HOST.md §5). The pointer stays valid for the manager's
    /// lifetime: entries are never erased.
    [[nodiscard]] const std::vector<PluginParameterInfo>* parametersFor(
        const std::string& uid) const;

    /// Consumes every live instance's latency-changed flag (the
    /// clap_host_latency callback), re-reading the changed ones. True when
    /// any figure changed — the caller's cue to recompile, which is how the
    /// new latency reaches delay compensation (docs/PLUGIN_HOST.md §8).
    /// Main thread.
    [[nodiscard]] bool refreshChangedLatencies();

    /// Libraries opened so far. The caching assertion in the tests.
    [[nodiscard]] std::size_t loadedLibraryCount() const;

    /// Instances alive right now. The persistence assertion in the tests.
    [[nodiscard]] std::size_t liveInstanceCount() const;

private:
    struct Held {
        std::unique_ptr<HostedPlugin> instance;
        std::string                   uid;
        double                        sampleRate = 0.0;
        std::uint32_t                 maxFrames  = 0;
    };

    /// Caller holds `mutex_`.
    [[nodiscard]] ClapLibrary* libraryFor(const std::string& path, std::string& error);

    const PluginRegistry* registry_ = nullptr;

    std::unordered_map<std::string, std::unique_ptr<ClapLibrary>>     libraries_;
    std::unordered_map<std::uint64_t, Held>                           instances_;
    std::unordered_map<std::string, std::vector<PluginParameterInfo>> parameters_;
    mutable std::mutex                                                mutex_;
};

} // namespace incdaw::plugins
