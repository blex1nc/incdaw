#pragma once

#include "engine/dsp/effects/BuiltinEffect.h"

#include <string>
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

    /// The presets INCDAW ships for this instrument (A5), in the same
    /// borrowed-static-storage terms the effect catalogue uses.
    dsp::FactoryPresetTable presets;
};

[[nodiscard]] const std::vector<BuiltinInstrumentInfo>& builtinInstruments();

/// Catalogue entry for `uid`, or nullptr — the instrument counterpart of
/// dsp::findBuiltinEffect, consumed by the instrument parameter panel.
[[nodiscard]] const BuiltinInstrumentInfo* findBuiltinInstrument(const std::string& uid);

} // namespace incdaw::engine
