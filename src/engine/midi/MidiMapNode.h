#pragma once

#include "engine/automation/AutomationNode.h"
#include "engine/graph/Node.h"

#include <vector>

namespace incdaw::engine {

/// Applies mapped MIDI controllers to parameters, once per message.
///
/// The mirror of AutomationNode: bindings are resolved at graph-compile time
/// through the same registry appliers, so a hardware knob and an automation
/// lane write through the same smoothed setters and cannot disagree about
/// what a parameter is. It reads the block's live MIDI from the context —
/// the same buffer the instruments receive — and produces no audio.
///
/// Editing mappings is a graph rebuild, exactly like editing an insert: the
/// bindings a node holds are immutable once the graph runs.
class MidiMapNode final : public Node {
public:
    struct Binding {
        int midiChannel = -1;   ///< -1 matches any channel
        int controller  = 0;    ///< CC number

        /// The mapped output range, normalised. min > max inverts the knob.
        float minValue = 0.0f;
        float maxValue = 1.0f;

        AutomationApplier apply;
    };

    void addBinding(Binding binding) { bindings_.push_back(std::move(binding)); }

    [[nodiscard]] std::size_t bindingCount() const noexcept { return bindings_.size(); }

    void process(const ProcessContext& context) noexcept override
    {
        if (context.liveMidi == nullptr)
            return;

        for (const MidiMessage& message : *context.liveMidi) {
            if (!message.isControlChange())
                continue;

            for (const Binding& binding : bindings_) {
                if (binding.controller != message.data1)
                    continue;
                if (binding.midiChannel >= 0 && binding.midiChannel != message.channel())
                    continue;
                if (!binding.apply)
                    continue;

                const float normalised = static_cast<float>(message.data2) / 127.0f;
                binding.apply(binding.minValue
                              + normalised * (binding.maxValue - binding.minValue));
            }
        }
    }

    [[nodiscard]] const char* name() const noexcept override { return "MidiMap"; }

private:
    std::vector<Binding> bindings_;
};

} // namespace incdaw::engine
