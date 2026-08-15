#pragma once

#include "engine/automation/AutomationNode.h"
#include "engine/dsp/MixerStripNode.h"
#include "engine/graph/ParameterSink.h"
#include "plugins/PluginParameterInfo.h"

#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace incdaw::project {

/// The parameter system: what can be automated, by key.
///
/// CLAUDE.md §10 forbids building automation separately per control, and the
/// roadmap's exit criterion is that a registered parameter is automatable with
/// NO parameter-specific code anywhere else. This registry is where the
/// per-parameter knowledge lives — one entry per key, each holding how a
/// normalised 0..1 value maps onto its target — and everything downstream (the
/// compiler, the AutomationNode, the commands, eventually recording and MIDI
/// learn) works purely in terms of keys and normalised values.
class ParameterRegistry {
public:
    /// Binds a normalised value onto a live strip. The strip pointer is owned
    /// by the graph the binding is compiled into; the applier must not be kept
    /// beyond it — which the AutomationNode guarantees by dying with the graph.
    using StripApplier = std::function<void(engine::dsp::MixerStripNode&, float)>;

    /// Binds a normalised value onto a parameter sink — a hosted plugin's
    /// instance. Same lifetime rule as StripApplier: the sink is owned by the
    /// graph the binding is compiled into.
    using SinkApplier = std::function<void(engine::ParameterSink&, float)>;

    /// Which alternative an entry holds decides what the compiler resolves the
    /// lane's target to: a strip rendering the target entity, or the sink of
    /// the insert slot the entity names.
    using Applier = std::variant<StripApplier, SinkApplier>;

    struct Entry {
        std::string key;
        Applier     apply;
    };

    /// The built-in parameters every strip has: "volume" and "pan".
    [[nodiscard]] static ParameterRegistry withBuiltins();

    /// The registry key for one plugin parameter. Scoped per plugin TYPE, not
    /// per instance: which instance a lane automates is its targetEntity (the
    /// insert slot), so two instances of one plugin share entries.
    [[nodiscard]] static std::string pluginParameterKey(const std::string& pluginUid,
                                                        std::uint32_t      parameterId);

    /// Registers or replaces a key. Registration is all a parameter needs to
    /// become automatable — that is the exit criterion, and the test for it
    /// registers a key this file has never heard of.
    void registerParameter(std::string key, Applier apply);

    /// Registers every discovered parameter of one plugin, keyed by
    /// pluginParameterKey. Each applier maps normalised 0..1 onto the
    /// parameter's plain range (stepped parameters land on integers) — generic
    /// per-KIND code, which is what keeps the "no parameter-specific code
    /// anywhere else" criterion intact for plugins.
    void registerPluginParameters(const std::string& pluginUid,
                                  const std::vector<plugins::PluginParameterInfo>& parameters);

    /// Registers every parameter of every builtin effect, from the engine's
    /// own catalogue, through exactly the same path hosted plugins use. The
    /// shell calls this once at launch; builtins never rescan.
    void registerBuiltinEffects();

    [[nodiscard]] const Entry* find(const std::string& key) const noexcept;

    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }

private:
    std::vector<Entry> entries_;
};

} // namespace incdaw::project
