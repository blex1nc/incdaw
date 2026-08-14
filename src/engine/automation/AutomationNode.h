#pragma once

#include "engine/automation/AutomationSequence.h"
#include "engine/graph/Node.h"
#include "engine/transport/TempoMap.h"

#include <functional>
#include <limits>
#include <vector>

namespace incdaw::engine {

/// Applies a value to a parameter. Bound at graph-compile time; called on the
/// audio thread, so it must not allocate — in practice it captures a node
/// pointer owned by the same graph and calls an atomic setter, and every such
/// setter is smoothed, so per-block evaluation cannot click.
using AutomationApplier = std::function<void(float)>;

/// Evaluates every automation lane once per block and writes the results.
///
/// A node rather than a thread or a timer: it rides the graph's execution
/// order, sees the same playPosition the instruments see, and dies with the
/// graph — so an applier can never outlive the node it writes to. It produces
/// no audio; its output stays silent.
///
/// This is the whole of CLAUDE.md §10's "generic subsystem": the node knows
/// nothing about what it is automating. Everything parameter-specific lives in
/// the applier bound by the registry at compile time.
class AutomationNode final : public Node {
public:
    struct Binding {
        AutomationSequence sequence;
        AutomationApplier  apply;

        /// The half-open tick range this binding writes in. Outside it the
        /// parameter is simply not touched — which is what gives a placed
        /// automation clip its natural semantics: nothing before it starts,
        /// and the value it left behind holds after it ends. The defaults
        /// mean "always", which is a lane placed nowhere (11a behaviour).
        Tick windowStart = std::numeric_limits<Tick>::min();
        Tick windowEnd   = std::numeric_limits<Tick>::max();
    };

    explicit AutomationNode(const TempoMap& tempoMap) noexcept : tempoMap_(&tempoMap) {}

    void addBinding(Binding binding) { bindings_.push_back(std::move(binding)); }

    [[nodiscard]] std::size_t bindingCount() const noexcept { return bindings_.size(); }

    void process(const ProcessContext& context) noexcept override
    {
        const Tick tick = tempoMap_->tickForFrame(context.playPosition);

        for (const Binding& binding : bindings_) {
            if (tick < binding.windowStart || tick >= binding.windowEnd)
                continue;

            if (!binding.sequence.isEmpty() && binding.apply)
                binding.apply(binding.sequence.valueAt(tick));
        }
    }

    [[nodiscard]] const char* name() const noexcept override { return "Automation"; }

private:
    const TempoMap*      tempoMap_;
    std::vector<Binding> bindings_;
};

} // namespace incdaw::engine
