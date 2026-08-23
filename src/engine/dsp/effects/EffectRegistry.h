#pragma once

#include "engine/dsp/effects/BuiltinEffects.h"

#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace incdaw::engine::dsp {

/// One row of the builtin-effect catalogue: what the application sees
/// (`info`) and how the compiler builds it (`make`).
///
/// `BuiltinEffects.cpp` owns the catalogue; each effect family contributes
/// its rows through a registrar. A family never reaches into the catalogue
/// directly, and the catalogue never names an individual effect — the
/// registrar is the only thing a new family has to add (docs/plugin-archive/
/// 00-CONTRACTS.md §3.1).
struct EffectCatalogueEntry {
    BuiltinEffectInfo info;
    std::function<std::unique_ptr<BuiltinEffect>(SampleRate)> make;
};

/// A family's entry point: appends one EffectCatalogueEntry per effect it
/// owns. Called once, in catalogue order, from `BuiltinEffects.cpp`.
using EffectRegistrar = void (*)(std::vector<EffectCatalogueEntry>&);

/// Appends one effect to `rows`.
///
/// Every effect knows its own parameter table; the row borrows it from a
/// throwaway instance so the catalogue cannot drift from the code. (The table
/// is static storage, so it outlives the probe.) `factory` takes the sample
/// rate and returns a `std::unique_ptr` to a BuiltinEffect subclass.
template <class Factory>
void addEffect(std::vector<EffectCatalogueEntry>& rows, const char* uid,
               const char* displayName, Factory factory)
{
    using Effect = typename decltype(factory(SampleRate{48000.0}))::element_type;
    static_assert(std::is_base_of_v<BuiltinEffect, Effect>);

    const auto probe = factory(SampleRate{48000.0});

    rows.push_back({{uid, displayName, probe->parameters(), probe->parameterCount()},
                    std::move(factory)});
}

// Each family declares its own registrar in its header
// (`void registerXxxEffects(std::vector<EffectCatalogueEntry>&)`) and is
// wired up by two lines in the marked blocks of BuiltinEffects.cpp.

} // namespace incdaw::engine::dsp
