#pragma once

#include "platform/AudioUnitHost.h"
#include "plugins/HostedPlugin.h"

#include <memory>
#include <string>

namespace incdaw::plugins {

/// An Audio Unit, hosted as a HostedPlugin.
///
/// Thin by design: everything CoreAudio lives in platform/AudioUnitHost, which
/// is what keeps this layer free of OS APIs (tools/check_layering.py enforces
/// it). This class is the translation — the format's parameter descriptions
/// become PluginParameterInfo, and its class-info blob becomes the same opaque
/// state every insert slot already knows how to store.
class AudioUnitInstance final : public HostedPlugin {
public:
    /// Instantiates `uid` ("type:subtype:manufacturer"). Returns nullptr with
    /// `error` set, like every other insert that cannot be built.
    [[nodiscard]] static std::unique_ptr<AudioUnitInstance> create(const std::string& uid,
                                                                   double        sampleRate,
                                                                   std::uint32_t maxFrames,
                                                                   std::string&  error);

    [[nodiscard]] bool process(float* left, float* right, std::uint32_t frames) noexcept override;

    /// Audio thread. AudioUnitSetParameter is the format's own realtime path,
    /// so unlike CLAP there is no queue to drain: the value goes straight in.
    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override;

    [[nodiscard]] const std::vector<PluginParameterInfo>& parameters() const noexcept override
    {
        return parameters_;
    }

    [[nodiscard]] std::uint32_t latencyFrames() const noexcept override;

    [[nodiscard]] bool saveState(std::vector<std::uint8_t>& out) const override;
    [[nodiscard]] bool loadState(const std::uint8_t* data, std::size_t size) override;

    [[nodiscard]] bool hasEditor() const noexcept override;
    [[nodiscard]] bool openEditor(void* parentView, std::uint32_t& width,
                                  std::uint32_t& height) override;
    void closeEditor() noexcept override;

private:
    AudioUnitInstance() = default;

    std::unique_ptr<platform::AudioUnitHandle> unit_;
    std::vector<PluginParameterInfo>           parameters_;
};

} // namespace incdaw::plugins
