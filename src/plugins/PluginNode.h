#pragma once

#include "engine/graph/Node.h"
#include "plugins/clap/ClapLibrary.h"

#include <cstring>
#include <memory>

namespace incdaw::plugins {

/// A hosted plugin as a render-graph node — the insert chain's building
/// block. Sums its graph inputs (like a strip does), then the plugin
/// processes the result in place on the node's output.
///
/// Ownership: the node owns the INSTANCE; the ClapLibrary the instance came
/// from must outlive the node, because destroying the instance calls into
/// the library's code. The instance manager that owns libraries for the
/// application's lifetime arrives with the compiler wiring; tests keep the
/// library alive on the stack.
///
/// Stereo today, like the rest of the graph. A mono graph passes through
/// untouched rather than handing the plugin two aliases of one buffer.
class PluginNode final : public engine::Node {
public:
    explicit PluginNode(std::unique_ptr<ClapInstance> instance) noexcept
        : instance_(std::move(instance)) {}

    void process(const engine::ProcessContext& context) noexcept override
    {
        const auto frames = context.frameCount;

        // Sum every source into the output, exactly as the mixer strip
        // sums: the plugin then sees the mixed signal, which is what an
        // insert means.
        for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel) {
            engine::Sample* out = context.output.channel(channel);

            for (std::size_t input = 0; input < context.inputCount; ++input) {
                const auto view = context.input(input);
                if (channel >= view.channelCount())
                    continue;

                const engine::Sample* source = view.channel(channel);
                for (engine::FrameCount frame = 0; frame < frames; ++frame)
                    out[frame] += source[frame];
            }
        }

        if (instance_ == nullptr || context.output.channelCount() < 2)
            return;   // pass-through: an honest no-op beats aliased buffers

        (void)instance_->process(context.output.channel(0), context.output.channel(1),
                                 static_cast<std::uint32_t>(frames));
    }

    [[nodiscard]] const char* name() const noexcept override { return "Plugin"; }

    /// Automation reaches the hosted plugin through here: the instance is the
    /// sink, turning values into CLAP events for its next process call.
    [[nodiscard]] engine::ParameterSink* parameterSink() noexcept override
    {
        return instance_.get();
    }

    /// Project save and load reach the hosted plugin's opaque state blob
    /// through here (docs/PLUGIN_HOST.md §6).
    [[nodiscard]] engine::StateIO* stateIO() noexcept override { return instance_.get(); }

private:
    std::unique_ptr<ClapInstance> instance_;
};

} // namespace incdaw::plugins
