#pragma once

#include "engine/dsp/effects/BuiltinEffect.h"

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
};

[[nodiscard]] const std::vector<BuiltinEffectInfo>& builtinEffects();

/// The effect for `uid`, or nullptr for an unknown one. The node carries its
/// ParameterSink and StateIO like any insert.
[[nodiscard]] std::unique_ptr<Node> makeBuiltinEffect(const std::string& uid,
                                                      SampleRate sampleRate);

/// Catalogue entry for `uid`, or nullptr.
[[nodiscard]] const BuiltinEffectInfo* findBuiltinEffect(const std::string& uid);

} // namespace incdaw::engine::dsp
