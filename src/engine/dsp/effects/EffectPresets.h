#pragma once

#include "engine/dsp/effects/BuiltinEffect.h"

#include <string_view>

namespace incdaw::engine::dsp {

/// The factory presets of one builtin effect, by catalogue uid.
///
/// Kept beside the effects rather than inside the catalogue table for the
/// same reason the parameter tables are: a preset is written in the effect's
/// own parameter ids, and an id that means "Drive" here means "Ceiling" two
/// rows down. An unknown uid, or one whose effect has no parameters, yields
/// an empty table — never nullptr rows with a non-zero count.
///
/// No preset stores the plain defaults: the catalogue already carries those,
/// and PresetLibrary synthesises the "Default" entry from them so that two
/// spellings of the same numbers cannot drift apart.
[[nodiscard]] FactoryPresetTable effectFactoryPresets(std::string_view uid);

} // namespace incdaw::engine::dsp
