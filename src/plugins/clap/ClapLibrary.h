#pragma once

#include "engine/core/LockFreeQueue.h"
#include "engine/graph/ParameterSink.h"
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
class ClapInstance final : public engine::ParameterSink {
public:
    ~ClapInstance() override;

    ClapInstance(const ClapInstance&)            = delete;
    ClapInstance& operator=(const ClapInstance&) = delete;

    /// Processes one stereo block in place through the plugin. Buffers are
    /// the host's; the plugin sees them as one input and one output port.
    /// Parameter values queued since the previous call are delivered first,
    /// in the block's input event list.
    [[nodiscard]] bool process(float* left, float* right, std::uint32_t frames) noexcept;

    /// Queues one PLAIN value for delivery in the next process() block.
    /// Realtime-safe: a lock-free push, no allocation. A full queue drops the
    /// value — automation writes every block, so the next block heals it.
    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override;

    /// The automatable parameters discovered at creation (CLAP_EXT_PARAMS),
    /// in plain terms. Empty for a plugin without the extension.
    [[nodiscard]] const std::vector<PluginParameterInfo>& parameters() const noexcept
    {
        return parameters_;
    }

    [[nodiscard]] const clap_plugin_t* raw() const noexcept { return plugin_; }

private:
    friend class ClapLibrary;
    ClapInstance() = default;

    struct ParamEvent {
        std::uint32_t id    = 0;
        double        value = 0.0;
    };

    const clap_plugin_t* plugin_ = nullptr;
    clap_host_t          host_{};
    bool                 processing_ = false;
    std::int64_t         steadyTime_ = 0;

    /// 256 slots is every automatable parameter of a large plugin changing in
    /// one block — sized for the worst block, not the common one. Preallocated
    /// with the instance; the audio thread only pushes and pops.
    engine::LockFreeQueue<ParamEvent, 256>  paramEvents_;
    std::vector<PluginParameterInfo>        parameters_;
};

} // namespace incdaw::plugins
