#pragma once

#include "engine/dsp/effects/BuiltinEffect.h"
#include "engine/transport/TempoMap.h"

#include <memory>
#include <string>
#include <vector>

namespace incdaw::engine::dsp {

/// The catalogue of builtin effects: one table, consulted by the compiler
/// (to build a slot whose identity is builtin), the parameter registry (to
/// make every builtin parameter automatable) and the UI (to list them). No
/// caller special-cases an individual effect — Phase 15's exit criterion.
struct BuiltinEffectInfo {
    const char* uid;           ///< e.g. "incdaw.eq" — what a PluginSlot stores
    const char* displayName;   ///< what a menu shows

    const EffectParameter* parameters;
    std::size_t            parameterCount;

    /// The presets INCDAW ships for this effect (A5). Borrowed static
    /// storage, like `parameters` — and empty for an effect with none.
    FactoryPresetTable presets;
};

[[nodiscard]] const std::vector<BuiltinEffectInfo>& builtinEffects();

/// The effect for `uid`, or nullptr for an unknown one. The node carries its
/// ParameterSink and StateIO like any insert.
///
/// `tempoMap` is for the one effect that syncs to the timeline; it must
/// outlive the node, which in a compiled project graph the graph's own map
/// does. Everything else ignores it, and a null map simply means a bar-synced
/// effect has nothing to sync to and passes the signal through.
[[nodiscard]] std::unique_ptr<Node> makeBuiltinEffect(const std::string& uid,
                                                      SampleRate      sampleRate,
                                                      const TempoMap* tempoMap = nullptr);

/// Catalogue entry for `uid`, or nullptr.
[[nodiscard]] const BuiltinEffectInfo* findBuiltinEffect(const std::string& uid);

} // namespace incdaw::engine::dsp
