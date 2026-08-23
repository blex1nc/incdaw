#include "engine/dsp/effects/BuiltinEffects.h"

#include "engine/dsp/effects/EffectRegistry.h"

#include "engine/dsp/effects/DynamicsEffects.h"
#include "engine/dsp/effects/ModulationEffects.h"
#include "engine/dsp/effects/SpaceEffects.h"
#include "engine/dsp/effects/ToneEffects.h"
#include "engine/dsp/effects/UtilityEffects.h"
// <<< incdaw:registrars:effects:include — one line per family
// >>> incdaw:registrars:effects:include

namespace incdaw::engine::dsp {

namespace {

/// One row per effect; everything else in the application derives from it.
/// The rows come from the families, in the order below — which is also the
/// order menus list them in.
const std::vector<EffectCatalogueEntry>& catalogue()
{
    static const std::vector<EffectCatalogueEntry> entries = [] {
        std::vector<EffectCatalogueEntry> rows;

        // <<< incdaw:registrars:effects — one line per family
        registerDynamicsEffects(rows);
        registerToneEffects(rows);
        registerSpaceEffects(rows);
        registerModulationEffects(rows);
        registerUtilityEffects(rows);
        // >>> incdaw:registrars:effects

        return rows;
    }();

    return entries;
}

} // namespace

const std::vector<BuiltinEffectInfo>& builtinEffects()
{
    static const std::vector<BuiltinEffectInfo> infos = [] {
        std::vector<BuiltinEffectInfo> rows;
        for (const EffectCatalogueEntry& entry : catalogue())
            rows.push_back(entry.info);
        return rows;
    }();

    return infos;
}

std::unique_ptr<Node> makeBuiltinEffect(const std::string& uid, SampleRate sampleRate)
{
    for (const EffectCatalogueEntry& entry : catalogue())
        if (uid == entry.info.uid)
            return entry.make(sampleRate);

    return nullptr;
}

const BuiltinEffectInfo* findBuiltinEffect(const std::string& uid)
{
    for (const BuiltinEffectInfo& info : builtinEffects())
        if (uid == info.uid)
            return &info;

    return nullptr;
}

} // namespace incdaw::engine::dsp
