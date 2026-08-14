#pragma once

#include "engine/automation/AutomationNode.h"
#include "engine/dsp/MixerStripNode.h"

#include <functional>
#include <string>
#include <vector>

namespace incdaw::project {

/// The parameter system: what can be automated, by key.
///
/// CLAUDE.md §10 forbids building automation separately per control, and the
/// roadmap's exit criterion is that a registered parameter is automatable with
/// NO parameter-specific code anywhere else. This registry is where the
/// per-parameter knowledge lives — one entry per key, each holding how a
/// normalised 0..1 value maps onto a strip — and everything downstream (the
/// compiler, the AutomationNode, the commands, eventually recording and MIDI
/// learn) works purely in terms of keys and normalised values.
class ParameterRegistry {
public:
    /// Binds a normalised value onto a live strip. The strip pointer is owned
    /// by the graph the binding is compiled into; the applier must not be kept
    /// beyond it — which the AutomationNode guarantees by dying with the graph.
    using StripApplier = std::function<void(engine::dsp::MixerStripNode&, float)>;

    struct Entry {
        std::string  key;
        StripApplier apply;
    };

    /// The built-in parameters every strip has: "volume" and "pan".
    [[nodiscard]] static ParameterRegistry withBuiltins();

    /// Registers or replaces a key. Registration is all a parameter needs to
    /// become automatable — that is the exit criterion, and the test for it
    /// registers a key this file has never heard of.
    void registerParameter(std::string key, StripApplier apply);

    [[nodiscard]] const Entry* find(const std::string& key) const noexcept;

    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }

private:
    std::vector<Entry> entries_;
};

} // namespace incdaw::project
