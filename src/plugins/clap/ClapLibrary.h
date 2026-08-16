#pragma once

#include "engine/core/LockFreeQueue.h"
#include "engine/graph/ParameterSink.h"
#include "engine/graph/StateIO.h"
#include "plugins/HostedPlugin.h"
#include "platform/SharedLibrary.h"
#include "plugins/PluginParameterInfo.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// The CLAP C ABI, vendored at third_party/clap (MIT; docs/DECISIONS.md
// D-027). Nothing above plugins/ ever includes it.
#include <clap/clap.h>

namespace incdaw::plugins {

/// What scanning one CLAP library yields per plugin.
struct ClapDescriptor {
    std::string id;
    std::string name;
    std::string vendor;
    std::string version;
};

class ClapInstance;

/// One loaded .clap library: entry, factory, descriptors, instantiation.
///
/// Bound by the prime directive of docs/PLUGIN_HOST.md §2: everything a
/// plugin returns is hostile input. Loading happens IN this process — which
/// is why scanning unknown plugins goes through the out-of-process scanner
/// binary, and only libraries that survived it are ever opened here.
class ClapLibrary {
public:
    ClapLibrary() = default;
    ~ClapLibrary();

    ClapLibrary(const ClapLibrary&)            = delete;
    ClapLibrary& operator=(const ClapLibrary&) = delete;

    /// Loads and initialises the entry. A macOS bundle directory resolves to
    /// its inner binary; a flat dylib loads directly.
    [[nodiscard]] bool open(const std::filesystem::path& path, std::string& error);
    void close();

    [[nodiscard]] bool isOpen() const noexcept { return entry_ != nullptr; }

    [[nodiscard]] std::vector<ClapDescriptor> descriptors() const;

    /// Creates, initialises and activates a plugin, ready to process.
    [[nodiscard]] std::unique_ptr<ClapInstance> create(const std::string& pluginId,
                                                       double sampleRate,
                                                       std::uint32_t maxFrames,
                                                       std::string& error);

private:
    platform::SharedLibrary        library_;
    const clap_plugin_entry_t*     entry_   = nullptr;
    const clap_plugin_factory_t*   factory_ = nullptr;
};

/// One activated plugin. Destruction stops, deactivates and destroys it.
///
/// As an engine::ParameterSink it accepts parameter values on the audio
/// thread and delivers them to the plugin as clap_event_param_value events in
/// its next process() call — never through a direct call into the plugin,
/// which the CLAP params contract forbids while processing (clap/ext/params.h:
/// flush must not run concurrently with process).
///
/// As an engine::StateIO it captures and restores the plugin's opaque state
/// blob through CLAP_EXT_STATE — main-thread calls, used by project save and
/// load (docs/PLUGIN_HOST.md §6).
class ClapInstance final : public HostedPlugin {
public:
    ~ClapInstance() override;

    ClapInstance(const ClapInstance&)            = delete;
    ClapInstance& operator=(const ClapInstance&) = delete;

    /// Processes one stereo block in place through the plugin. Buffers are
    /// the host's; the plugin sees them as one input and one output port.
    /// Parameter values queued since the previous call are delivered first,
    /// in the block's input event list.
    [[nodiscard]] bool process(float* left, float* right, std::uint32_t frames) noexcept override;

    /// Queues one PLAIN value for delivery in the next process() block.
    /// Realtime-safe: a lock-free push, no allocation. A full queue drops the
    /// value — automation writes every block, so the next block heals it.
    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override;

    /// The automatable parameters discovered at creation (CLAP_EXT_PARAMS),
    /// in plain terms. Empty for a plugin without the extension.
    [[nodiscard]] const std::vector<PluginParameterInfo>& parameters() const noexcept override
    {
        return parameters_;
    }

    /// Processing delay the plugin reported through CLAP_EXT_LATENCY at
    /// creation, in frames. 0 for a plugin without the extension. Feeds
    /// PluginNode::latencyFrames, and from there the graph's existing delay
    /// compensation (docs/AUDIO_ENGINE.md §7).
    [[nodiscard]] std::uint32_t latencyFrames() const noexcept override { return latency_; }

    // ── The editor (CLAP_EXT_GUI, docs/PLUGIN_HOST.md §7) ────────────────
    // Main-thread only, like all lifecycle calls. `parentView` is an NSView*
    // as void*: this layer never includes Cocoa (platform containment), and
    // the CLAP window handle is a void* anyway.

    /// True when the plugin offers an embeddable Cocoa editor.
    [[nodiscard]] bool hasEditor() const noexcept override;

    /// Creates the editor embedded in `parentView` and reports the plugin's
    /// size. False leaves no editor behind, whichever step refused.
    [[nodiscard]] bool openEditor(void* parentView, std::uint32_t& width,
                                  std::uint32_t& height) override;

    /// Destroys the editor if one is open. Safe to call twice; called by the
    /// destructor, so a closing window and a dying instance cannot double-free.
    void closeEditor() noexcept override;

    [[nodiscard]] bool isEditorOpen() const noexcept { return editorOpen_; }

    /// Captures the plugin's opaque state (CLAP_EXT_STATE). False when the
    /// plugin has no state extension, refuses, or writes beyond the size cap
    /// a hostile plugin is held to — the caller keeps its previous blob.
    [[nodiscard]] bool saveState(std::vector<std::uint8_t>& out) const override;

    /// Hands a previously captured blob back to the plugin. The blob is
    /// untrusted on principle, but it is the PLUGIN's job to validate its own
    /// serialisation; false means it refused.
    [[nodiscard]] bool loadState(const std::uint8_t* data, std::size_t size) override;

    [[nodiscard]] const clap_plugin_t* raw() const noexcept { return plugin_; }

private:
    friend class ClapLibrary;
    ClapInstance() = default;

    struct ParamEvent {
        std::uint32_t id    = 0;
        double        value = 0.0;
    };

    const clap_plugin_t*       plugin_ = nullptr;
    const clap_plugin_state_t* state_  = nullptr;   ///< CLAP_EXT_STATE, or null
    const clap_plugin_gui_t*   gui_    = nullptr;   ///< CLAP_EXT_GUI, or null
    clap_host_t                host_{};
    bool                       processing_ = false;
    bool                       editorOpen_ = false;
    std::int64_t               steadyTime_ = 0;
    std::uint32_t              latency_    = 0;

    /// 256 slots is every automatable parameter of a large plugin changing in
    /// one block — sized for the worst block, not the common one. Preallocated
    /// with the instance; the audio thread only pushes and pops.
    engine::LockFreeQueue<ParamEvent, 256>  paramEvents_;
    std::vector<PluginParameterInfo>        parameters_;
};

} // namespace incdaw::plugins
