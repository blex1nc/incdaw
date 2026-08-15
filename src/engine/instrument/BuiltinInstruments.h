#pragma once

#include "engine/dsp/effects/BuiltinEffect.h"

#include <vector>

namespace incdaw::engine {

/// The catalogue of builtin instruments' parameters — the instrument
/// counterpart of the effect catalogue, consumed by the parameter registry
/// so an instrument's cutoff is automatable and MIDI-mappable through the
/// same machinery as everything else. Construction is NOT here: instruments
/// are built by the compiler, which needs decoded assets the catalogue
/// cannot know about.
struct BuiltinInstrumentInfo {
    const char* uid;           ///< matches the channel's PluginIdentifier uid
    const char* displayName;

    const dsp::EffectParameter* parameters;
    std::size_t                 parameterCount;
};

[[nodiscard]] const std::vector<BuiltinInstrumentInfo>& builtinInstruments();

} // namespace incdaw::engine
