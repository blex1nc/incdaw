#include "engine/dsp/effects/EffectPresets.h"

#include "engine/dsp/effects/DynamicsEffects.h"
#include "engine/dsp/effects/ModulationEffects.h"
#include "engine/dsp/effects/MultibandEffects.h"
#include "engine/dsp/effects/SpaceEffects.h"
#include "engine/dsp/effects/ToneEffects.h"
#include "engine/dsp/effects/UtilityEffects.h"

#include <iterator>

namespace incdaw::engine::dsp {

namespace {

// ── Utility ──────────────────────────────────────────────────────────────────

constexpr PresetValue utilityMono[]   = {{UtilityEffect::mono, 1.0}};
constexpr PresetValue utilityWide[]   = {{UtilityEffect::width, 1.45}};
constexpr PresetValue utilityNarrow[] = {{UtilityEffect::width, 0.55}};
constexpr PresetValue utilityTrim[]   = {{UtilityEffect::gainDb, -6.0}};

constexpr FactoryPreset utilityPresets[] = {
    {"Mono Check", utilityMono,   std::size(utilityMono)},
    {"Wider",      utilityWide,   std::size(utilityWide)},
    {"Narrower",   utilityNarrow, std::size(utilityNarrow)},
    {"Trim -6 dB", utilityTrim,   std::size(utilityTrim)},
};

// ── Filter ───────────────────────────────────────────────────────────────────

constexpr PresetValue filterLowpass[] = {
    {FilterEffect::mode, static_cast<double>(FilterEffect::lowpass)},
    {FilterEffect::cutoffHz, 900.0},
    {FilterEffect::resonance, 1.2},
};
constexpr PresetValue filterRumbleCut[] = {
    {FilterEffect::mode, static_cast<double>(FilterEffect::highpass)},
    {FilterEffect::cutoffHz, 80.0},
    {FilterEffect::resonance, 0.7071},
};
constexpr PresetValue filterTelephone[] = {
    {FilterEffect::mode, static_cast<double>(FilterEffect::bandpass)},
    {FilterEffect::cutoffHz, 1400.0},
    {FilterEffect::resonance, 2.5},
};

constexpr FactoryPreset filterPresets[] = {
    {"Lowpass Sweep", filterLowpass,   std::size(filterLowpass)},
    {"Rumble Cut",    filterRumbleCut, std::size(filterRumbleCut)},
    {"Telephone",     filterTelephone, std::size(filterTelephone)},
};

// ── EQ, and the same filter under the desk face ──────────────────────────────

constexpr PresetValue eqAir[] = {
    {EqEffect::highFreq, 10000.0},
    {EqEffect::highGainDb, 3.0},
};
constexpr PresetValue eqTightLows[] = {
    {EqEffect::lowFreq, 120.0},
    {EqEffect::lowGainDb, -4.0},
};
constexpr PresetValue eqPresence[] = {
    {EqEffect::midFreq, 3000.0},
    {EqEffect::midGainDb, 3.0},
    {EqEffect::midQ, 1.0},
};
constexpr PresetValue eqSmile[] = {
    {EqEffect::lowGainDb, 3.0},
    {EqEffect::midFreq, 800.0},
    {EqEffect::midGainDb, -2.5},
    {EqEffect::highGainDb, 3.0},
};

constexpr FactoryPreset eqPresets[] = {
    {"Air Lift",    eqAir,       std::size(eqAir)},
    {"Tighten Low", eqTightLows, std::size(eqTightLows)},
    {"Presence",    eqPresence,  std::size(eqPresence)},
    {"Smile",       eqSmile,     std::size(eqSmile)},
};

constexpr PresetValue toneWarm[] = {
    {EqEffect::lowGainDb, 3.0},
    {EqEffect::highGainDb, -2.0},
};
constexpr PresetValue toneBright[] = {
    {EqEffect::lowGainDb, -1.0},
    {EqEffect::highGainDb, 4.0},
};
constexpr PresetValue toneScooped[] = {
    {EqEffect::midFreq, 700.0},
    {EqEffect::midGainDb, -5.0},
};

constexpr FactoryPreset tonePresets[] = {
    {"Warm",    toneWarm,    std::size(toneWarm)},
    {"Bright",  toneBright,  std::size(toneBright)},
    {"Scooped", toneScooped, std::size(toneScooped)},
};

// ── Saturator ────────────────────────────────────────────────────────────────

constexpr PresetValue saturatorSubtle[] = {
    {SaturatorEffect::driveDb, 6.0},
    {SaturatorEffect::mix, 0.5},
};
constexpr PresetValue saturatorCrunch[] = {
    {SaturatorEffect::driveDb, 18.0},
    {SaturatorEffect::mix, 1.0},
};
constexpr PresetValue saturatorParallel[] = {
    {SaturatorEffect::driveDb, 24.0},
    {SaturatorEffect::mix, 0.35},
};

constexpr FactoryPreset saturatorPresets[] = {
    {"Subtle Drive",  saturatorSubtle,   std::size(saturatorSubtle)},
    {"Crunch",        saturatorCrunch,   std::size(saturatorCrunch)},
    {"Parallel Grit", saturatorParallel, std::size(saturatorParallel)},
};

// ── Dynamics ─────────────────────────────────────────────────────────────────

constexpr PresetValue compressorVocal[] = {
    {CompressorEffect::thresholdDb, -18.0},
    {CompressorEffect::ratio, 3.0},
    {CompressorEffect::attackMs, 8.0},
    {CompressorEffect::releaseMs, 120.0},
    {CompressorEffect::makeupDb, 4.0},
};
constexpr PresetValue compressorGlue[] = {
    {CompressorEffect::thresholdDb, -12.0},
    {CompressorEffect::ratio, 2.0},
    {CompressorEffect::attackMs, 20.0},
    {CompressorEffect::releaseMs, 200.0},
    {CompressorEffect::makeupDb, 2.0},
};
constexpr PresetValue compressorPeaks[] = {
    {CompressorEffect::thresholdDb, -6.0},
    {CompressorEffect::ratio, 8.0},
    {CompressorEffect::attackMs, 1.0},
    {CompressorEffect::releaseMs, 80.0},
    {CompressorEffect::makeupDb, 1.0},
};

constexpr FactoryPreset compressorPresets[] = {
    {"Vocal Levelling", compressorVocal, std::size(compressorVocal)},
    {"Bus Glue",        compressorGlue,  std::size(compressorGlue)},
    {"Peak Tamer",      compressorPeaks, std::size(compressorPeaks)},
};

constexpr PresetValue limiterTransparent[] = {
    {LimiterEffect::ceilingDb, -0.3},
    {LimiterEffect::releaseMs, 100.0},
};
constexpr PresetValue limiterLoud[] = {
    {LimiterEffect::ceilingDb, -0.1},
    {LimiterEffect::releaseMs, 20.0},
};

constexpr FactoryPreset limiterPresets[] = {
    {"Transparent", limiterTransparent, std::size(limiterTransparent)},
    {"Loud",        limiterLoud,        std::size(limiterLoud)},
};

// The lookahead limiter's ids happen to match the feedback one's, but writing
// them out again is the point: two effects sharing a preset table would break
// the moment either grew a parameter.
constexpr PresetValue lookaheadTransparent[] = {
    {LookaheadLimiterEffect::ceilingDb, -0.3},
    {LookaheadLimiterEffect::releaseMs, 100.0},
};
constexpr PresetValue lookaheadLoud[] = {
    {LookaheadLimiterEffect::ceilingDb, -0.1},
    {LookaheadLimiterEffect::releaseMs, 20.0},
};

constexpr FactoryPreset lookaheadPresets[] = {
    {"Transparent", lookaheadTransparent, std::size(lookaheadTransparent)},
    {"Mastering",   lookaheadLoud,        std::size(lookaheadLoud)},
};

constexpr PresetValue gateDrum[] = {
    {GateEffect::thresholdDb, -40.0},
    {GateEffect::attackMs, 0.5},
    {GateEffect::holdMs, 40.0},
    {GateEffect::releaseMs, 120.0},
};
constexpr PresetValue gateNoiseFloor[] = {
    {GateEffect::thresholdDb, -55.0},
    {GateEffect::attackMs, 5.0},
    {GateEffect::holdMs, 200.0},
    {GateEffect::releaseMs, 400.0},
};

constexpr FactoryPreset gatePresets[] = {
    {"Tight Drum",  gateDrum,       std::size(gateDrum)},
    {"Noise Floor", gateNoiseFloor, std::size(gateNoiseFloor)},
};

constexpr PresetValue transientPunch[] = {
    {TransientSplitEffect::output, static_cast<double>(TransientSplitEffect::Output::both)},
    {TransientSplitEffect::transientDb, 4.0},
    {TransientSplitEffect::sustainDb, -2.0},
};
constexpr PresetValue transientRoomKill[] = {
    {TransientSplitEffect::output, static_cast<double>(TransientSplitEffect::Output::both)},
    {TransientSplitEffect::transientDb, 0.0},
    {TransientSplitEffect::sustainDb, -8.0},
};
constexpr PresetValue transientOnly[] = {
    {TransientSplitEffect::output,
     static_cast<double>(TransientSplitEffect::Output::transientsOnly)},
};

constexpr FactoryPreset transientPresets[] = {
    {"Punch Up",        transientPunch,    std::size(transientPunch)},
    {"Kill the Room",   transientRoomKill, std::size(transientRoomKill)},
    {"Transients Only", transientOnly,     std::size(transientOnly)},
};

// ── Space ────────────────────────────────────────────────────────────────────

constexpr PresetValue delaySlapback[] = {
    {DelayEffect::timeMs, 110.0},
    {DelayEffect::feedback, 0.1},
    {DelayEffect::mix, 0.25},
};
constexpr PresetValue delayEighth[] = {
    {DelayEffect::timeMs, 250.0},
    {DelayEffect::feedback, 0.35},
    {DelayEffect::mix, 0.3},
};
constexpr PresetValue delayDub[] = {
    {DelayEffect::timeMs, 375.0},
    {DelayEffect::feedback, 0.7},
    {DelayEffect::mix, 0.4},
};

constexpr FactoryPreset delayPresets[] = {
    {"Slapback",  delaySlapback, std::size(delaySlapback)},
    {"Eighth",    delayEighth,   std::size(delayEighth)},
    {"Dub Echo",  delayDub,      std::size(delayDub)},
};

constexpr PresetValue reverbRoom[] = {
    {ReverbEffect::size, 0.35},
    {ReverbEffect::damping, 0.6},
    {ReverbEffect::mix, 0.2},
};
constexpr PresetValue reverbPlate[] = {
    {ReverbEffect::size, 0.8},
    {ReverbEffect::damping, 0.25},
    {ReverbEffect::mix, 0.3},
};
constexpr PresetValue reverbHall[] = {
    {ReverbEffect::size, 1.3},
    {ReverbEffect::damping, 0.4},
    {ReverbEffect::mix, 0.35},
};

constexpr FactoryPreset reverbPresets[] = {
    {"Small Room", reverbRoom,  std::size(reverbRoom)},
    {"Plate",      reverbPlate, std::size(reverbPlate)},
    {"Hall",       reverbHall,  std::size(reverbHall)},
};

// ── Modulation ───────────────────────────────────────────────────────────────

constexpr PresetValue chorusSubtle[] = {
    {ChorusEffect::rateHz, 0.4},
    {ChorusEffect::depthMs, 2.0},
    {ChorusEffect::mix, 0.25},
};
constexpr PresetValue chorusLush[] = {
    {ChorusEffect::rateHz, 0.8},
    {ChorusEffect::depthMs, 6.0},
    {ChorusEffect::mix, 0.5},
};

constexpr FactoryPreset chorusPresets[] = {
    {"Subtle", chorusSubtle, std::size(chorusSubtle)},
    {"Lush",   chorusLush,   std::size(chorusLush)},
};

constexpr PresetValue flangerJet[] = {
    {FlangerEffect::rateHz, 0.15},
    {FlangerEffect::depthMs, 3.0},
    {FlangerEffect::feedback, 0.8},
    {FlangerEffect::mix, 0.5},
};
constexpr PresetValue flangerLight[] = {
    {FlangerEffect::rateHz, 0.3},
    {FlangerEffect::depthMs, 1.5},
    {FlangerEffect::feedback, 0.3},
    {FlangerEffect::mix, 0.3},
};

constexpr FactoryPreset flangerPresets[] = {
    {"Jet",         flangerJet,   std::size(flangerJet)},
    {"Light Sweep", flangerLight, std::size(flangerLight)},
};

constexpr PresetValue phaserSlow[] = {
    {PhaserEffect::rateHz, 0.15},
    {PhaserEffect::feedback, 0.5},
    {PhaserEffect::mix, 0.4},
};
constexpr PresetValue phaserFast[] = {
    {PhaserEffect::rateHz, 1.2},
    {PhaserEffect::feedback, 0.3},
    {PhaserEffect::mix, 0.5},
};

constexpr FactoryPreset phaserPresets[] = {
    {"Slow Swirl",  phaserSlow, std::size(phaserSlow)},
    {"Fast Wobble", phaserFast, std::size(phaserFast)},
};

// ── Multiband ────────────────────────────────────────────────────────────────

using Multiband = MultibandCompressorEffect;

constexpr PresetValue multibandGlue[] = {
    {Multiband::crossoverLowHz, 180.0},
    {Multiband::crossoverHighHz, 2800.0},
    {Multiband::bandParameter(0, Multiband::bandThresholdDb), -14.0},
    {Multiband::bandParameter(0, Multiband::bandRatio), 2.0},
    {Multiband::bandParameter(1, Multiband::bandThresholdDb), -16.0},
    {Multiband::bandParameter(1, Multiband::bandRatio), 1.8},
    {Multiband::bandParameter(2, Multiband::bandThresholdDb), -18.0},
    {Multiband::bandParameter(2, Multiband::bandRatio), 2.2},
    {Multiband::outputDb, 2.0},
};
constexpr PresetValue multibandTameLows[] = {
    {Multiband::crossoverLowHz, 120.0},
    {Multiband::bandParameter(0, Multiband::bandThresholdDb), -20.0},
    {Multiband::bandParameter(0, Multiband::bandRatio), 4.0},
    {Multiband::bandParameter(0, Multiband::bandAttackMs), 30.0},
    {Multiband::bandParameter(0, Multiband::bandReleaseMs), 300.0},
};
constexpr PresetValue multibandDeHarsh[] = {
    {Multiband::crossoverHighHz, 4000.0},
    {Multiband::bandParameter(2, Multiband::bandThresholdDb), -26.0},
    {Multiband::bandParameter(2, Multiband::bandRatio), 5.0},
    {Multiband::bandParameter(2, Multiband::bandAttackMs), 1.0},
    {Multiband::bandParameter(2, Multiband::bandReleaseMs), 60.0},
};
constexpr PresetValue multibandPump[] = {
    {Multiband::bandParameter(0, Multiband::bandThresholdDb), -24.0},
    {Multiband::bandParameter(0, Multiband::bandRatio), 8.0},
    {Multiband::bandParameter(0, Multiband::bandAttackMs), 5.0},
    {Multiband::bandParameter(0, Multiband::bandReleaseMs), 250.0},
    {Multiband::bandParameter(0, Multiband::bandMakeupDb), 4.0},
};

constexpr FactoryPreset multibandPresets[] = {
    {"Bus Glue",     multibandGlue,     std::size(multibandGlue)},
    {"Tame the Low", multibandTameLows, std::size(multibandTameLows)},
    {"De-Harsh",     multibandDeHarsh,  std::size(multibandDeHarsh)},
    {"Low Pump",     multibandPump,     std::size(multibandPump)},
};

struct Row {
    std::string_view   uid;
    FactoryPresetTable table;
};

/// One row per catalogue entry that has parameters worth naming. The meters
/// (`incdaw.analyzer`, `incdaw.loudness`) are absent on purpose: they have no
/// parameters, so a preset of theirs would store nothing.
constexpr Row rows[] = {
    {"incdaw.utility",        {utilityPresets,    std::size(utilityPresets)}},
    {"incdaw.filter",         {filterPresets,     std::size(filterPresets)}},
    {"incdaw.eq",             {eqPresets,         std::size(eqPresets)}},
    {"incdaw.tone",           {tonePresets,       std::size(tonePresets)}},
    {"incdaw.saturator",      {saturatorPresets,  std::size(saturatorPresets)}},
    {"incdaw.compressor",     {compressorPresets, std::size(compressorPresets)}},
    {"incdaw.limiter",        {limiterPresets,    std::size(limiterPresets)}},
    {"incdaw.limiterla",      {lookaheadPresets,  std::size(lookaheadPresets)}},
    {"incdaw.gate",           {gatePresets,       std::size(gatePresets)}},
    {"incdaw.delay",          {delayPresets,      std::size(delayPresets)}},
    {"incdaw.reverb",         {reverbPresets,     std::size(reverbPresets)}},
    {"incdaw.chorus",         {chorusPresets,     std::size(chorusPresets)}},
    {"incdaw.flanger",        {flangerPresets,    std::size(flangerPresets)}},
    {"incdaw.phaser",         {phaserPresets,     std::size(phaserPresets)}},
    {"incdaw.transientsplit", {transientPresets,  std::size(transientPresets)}},
    {"incdaw.multiband",      {multibandPresets,  std::size(multibandPresets)}},
};

} // namespace

FactoryPresetTable effectFactoryPresets(std::string_view uid)
{
    for (const Row& row : rows)
        if (row.uid == uid)
            return row.table;

    return {};
}

} // namespace incdaw::engine::dsp
