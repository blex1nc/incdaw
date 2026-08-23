#include "engine/dsp/effects/BuiltinEffects.h"

#include "engine/dsp/effects/DynamicsEffects.h"
#include "engine/dsp/effects/EffectPresets.h"
#include "engine/dsp/effects/ModulationEffects.h"
#include "engine/dsp/effects/MultibandEffects.h"
#include "engine/dsp/effects/SpaceEffects.h"
#include "engine/dsp/effects/ToneEffects.h"
#include "engine/dsp/effects/UtilityEffects.h"

#include <functional>
#include <type_traits>

namespace incdaw::engine::dsp {

namespace {

struct CatalogueEntry {
    BuiltinEffectInfo info;
    std::function<std::unique_ptr<BuiltinEffect>(SampleRate)> make;
};

/// One row per effect; everything else in the application derives from it.
const std::vector<CatalogueEntry>& catalogue()
{
    static const std::vector<CatalogueEntry> entries = [] {
        std::vector<CatalogueEntry> rows;

        const auto add = [&rows](const char* uid, const char* displayName, auto factory) {
            using Effect = typename decltype(factory(SampleRate{48000.0}))::element_type;

            // Every effect knows its own parameter table; borrow it from a
            // throwaway instance so the catalogue cannot drift from the code.
            // (The table is static storage, so it outlives the probe.)
            static_assert(std::is_base_of_v<BuiltinEffect, Effect>);
            const auto probe = factory(SampleRate{48000.0});

            rows.push_back({{uid, displayName, probe->parameters(),
                             probe->parameterCount(), effectFactoryPresets(uid)},
                            std::move(factory)});
        };

        add("incdaw.utility",    "Utility",    [](SampleRate) { return std::make_unique<UtilityEffect>(); });
        add("incdaw.filter",     "Filter",     [](SampleRate) { return std::make_unique<FilterEffect>(); });
        add("incdaw.eq",         "EQ 3-Band",  [](SampleRate) { return std::make_unique<EqEffect>(); });

        // The same three-band EQ under a mixing-desk face: the shell gives
        // this uid a Bass/Mid/Treble panel with a response curve instead of
        // seven sliders. One uid, one DSP class — a second tone stack would
        // be the same filter written twice (CLAUDE.md §34).
        add("incdaw.tone",       "Tone (Bass/Mid/Treble)",
            [](SampleRate) { return std::make_unique<EqEffect>(); });
        add("incdaw.saturator",  "Saturator",  [](SampleRate) { return std::make_unique<SaturatorEffect>(); });
        add("incdaw.compressor", "Compressor", [](SampleRate) { return std::make_unique<CompressorEffect>(); });
        add("incdaw.limiter",    "Limiter",    [](SampleRate) { return std::make_unique<LimiterEffect>(); });
        add("incdaw.gate",       "Gate",       [](SampleRate) { return std::make_unique<GateEffect>(); });
        add("incdaw.delay",      "Delay",      [](SampleRate) { return std::make_unique<DelayEffect>(); });
        add("incdaw.reverb",     "Reverb",     [](SampleRate) { return std::make_unique<ReverbEffect>(); });
        add("incdaw.analyzer",   "Analyzer",   [](SampleRate) { return std::make_unique<AnalyzerEffect>(); });
        add("incdaw.limiterla",  "Limiter (Lookahead)",
            [](SampleRate rate) { return std::make_unique<LookaheadLimiterEffect>(rate); });
        add("incdaw.loudness",   "Loudness Meter",
            [](SampleRate) { return std::make_unique<LoudnessMeterEffect>(); });
        add("incdaw.chorus",     "Chorus",     [](SampleRate) { return std::make_unique<ChorusEffect>(); });
        add("incdaw.flanger",    "Flanger",    [](SampleRate) { return std::make_unique<FlangerEffect>(); });
        add("incdaw.phaser",     "Phaser",     [](SampleRate) { return std::make_unique<PhaserEffect>(); });
        add("incdaw.transientsplit", "Transient Split",
            [](SampleRate) { return std::make_unique<TransientSplitEffect>(); });
        add("incdaw.multiband", "Multiband Compressor",
            [](SampleRate) { return std::make_unique<MultibandCompressorEffect>(); });
        add("incdaw.deesser",   "De-Esser",
            [](SampleRate) { return std::make_unique<DeEsserEffect>(); });

        return rows;
    }();

    return entries;
}

} // namespace

const std::vector<BuiltinEffectInfo>& builtinEffects()
{
    static const std::vector<BuiltinEffectInfo> infos = [] {
        std::vector<BuiltinEffectInfo> rows;
        for (const CatalogueEntry& entry : catalogue())
            rows.push_back(entry.info);
        return rows;
    }();

    return infos;
}

std::unique_ptr<Node> makeBuiltinEffect(const std::string& uid, SampleRate sampleRate)
{
    for (const CatalogueEntry& entry : catalogue())
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
