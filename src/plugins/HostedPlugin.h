#pragma once

#include "engine/graph/ParameterSink.h"
#include "engine/graph/StateIO.h"
#include "plugins/PluginParameterInfo.h"

#include <cstdint>
#include <vector>

namespace incdaw::plugins {

/// One live plugin, whatever format it came from.
///
/// Everything above this interface — PluginNode, the instance manager, the
/// shell's editor windows, delay compensation, state files, the parameter
/// registry — is written against it and knows nothing about CLAP or Audio
/// Units. Adding a format means implementing this and nothing else, which is
/// the whole point: the second format is where a host either has an interface
/// or grows a copy of itself (docs/PLUGIN_HOST.md).
///
/// As an engine::ParameterSink it takes parameter values on the audio thread;
/// as an engine::StateIO it captures and restores an opaque blob on the main
/// thread. How each format satisfies those contracts is its own business —
/// CLAP queues events for its next process() call, an Audio Unit sets the
/// parameter directly, and both are correct for their format.
class HostedPlugin : public engine::ParameterSink, public engine::StateIO {
public:
    ~HostedPlugin() override = default;

    /// Audio thread. Processes one stereo block in place.
    [[nodiscard]] virtual bool process(float* left, float* right, std::uint32_t frames) noexcept = 0;

    /// The automatable parameters discovered when the instance was created.
    [[nodiscard]] virtual const std::vector<PluginParameterInfo>& parameters() const noexcept = 0;

    /// Reported processing delay in frames, for the graph's compensation.
    [[nodiscard]] virtual std::uint32_t latencyFrames() const noexcept = 0;

    [[nodiscard]] virtual bool hasEditor() const noexcept = 0;

    /// Embeds an editor in `parentView` (an NSView*), reporting the size it
    /// wants. False leaves no editor behind, whichever step refused.
    [[nodiscard]] virtual bool openEditor(void* parentView, std::uint32_t& width,
                                          std::uint32_t& height) = 0;

    virtual void closeEditor() noexcept = 0;
};

} // namespace incdaw::plugins
