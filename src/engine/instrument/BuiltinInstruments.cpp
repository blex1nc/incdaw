#include "engine/instrument/BuiltinInstruments.h"

#include "engine/instrument/PianoInstrument.h"
#include "engine/instrument/Sampler.h"
#include "engine/instrument/SimpleSynth.h"
#include "engine/instrument/WavetableSynth.h"

#include <iterator>

namespace incdaw::engine {

namespace {

using dsp::EffectParameter;
using dsp::FactoryPreset;
using dsp::PresetValue;

constexpr EffectParameter samplerParameters[] = {
    {static_cast<std::uint32_t>(SamplerParam::attackSeconds),   "Attack",       0.0,    10.0,     0.002, false},
    {static_cast<std::uint32_t>(SamplerParam::decaySeconds),    "Decay",        0.0,    10.0,     0.05,  false},
    {static_cast<std::uint32_t>(SamplerParam::sustainLevel),    "Sustain",      0.0,     1.0,     1.0,   false},
    {static_cast<std::uint32_t>(SamplerParam::releaseSeconds),  "Release",      0.0,    10.0,     0.05,  false},
    {static_cast<std::uint32_t>(SamplerParam::filterMode),      "Filter Mode",  0.0,     3.0,     0.0,   true},
    {static_cast<std::uint32_t>(SamplerParam::filterCutoffHz),  "Cutoff",      20.0, 20000.0, 20000.0,   false},
    {static_cast<std::uint32_t>(SamplerParam::filterResonance), "Resonance",    0.1,    10.0,     0.7071,false},
    {static_cast<std::uint32_t>(SamplerParam::lfoRateHz),       "LFO Rate",     0.0,    20.0,     5.0,   false},
    {static_cast<std::uint32_t>(SamplerParam::lfoToPitch),      "LFO > Pitch", -24.0,   24.0,     0.0,   false},
    {static_cast<std::uint32_t>(SamplerParam::lfoToCutoff),     "LFO > Cutoff", -8.0,    8.0,     0.0,   false},
};

constexpr EffectParameter simpleSynthParameters[] = {
    {static_cast<std::uint32_t>(SimpleSynthParam::waveform),       "Waveform", 0.0,  3.0, 1.0, true},
    {static_cast<std::uint32_t>(SimpleSynthParam::gain),           "Gain",     0.0,  1.0, 0.5, false},
    {static_cast<std::uint32_t>(SimpleSynthParam::attackSeconds),  "Attack",   0.0, 10.0, 0.002, false},
    {static_cast<std::uint32_t>(SimpleSynthParam::decaySeconds),   "Decay",    0.0, 10.0, 0.05, false},
    {static_cast<std::uint32_t>(SimpleSynthParam::sustainLevel),   "Sustain",  0.0,  1.0, 1.0, false},
    {static_cast<std::uint32_t>(SimpleSynthParam::releaseSeconds), "Release",  0.0, 10.0, 0.05, false},
};

/// The piano's `Model` is stepped: it selects a voicing, and a value between
/// two of them is not half of each. The registry and the panel both read that
/// flag rather than each deciding for themselves.
constexpr EffectParameter pianoParameters[] = {
    {static_cast<std::uint32_t>(PianoParam::model),        "Model",        0.0,  4.0, 0.0,  true},
    {static_cast<std::uint32_t>(PianoParam::tone),         "Tone",        -1.0,  1.0, 0.0,  false},
    {static_cast<std::uint32_t>(PianoParam::hardness),     "Hardness",     0.0,  1.0, 0.5,  false},
    {static_cast<std::uint32_t>(PianoParam::decay),        "Decay",        0.1,  4.0, 1.0,  false},
    {static_cast<std::uint32_t>(PianoParam::release),      "Release",      0.1,  4.0, 1.0,  false},
    {static_cast<std::uint32_t>(PianoParam::stretchCents), "Stretch",      0.0, 50.0, 12.0, false},
    {static_cast<std::uint32_t>(PianoParam::hammerNoise),  "Hammer",       0.0,  1.0, 0.35, false},
    {static_cast<std::uint32_t>(PianoParam::pedalTail),    "Pedal Tail",   0.0,  1.0, 0.3,  false},
    {static_cast<std::uint32_t>(PianoParam::stereoSpread), "Spread",       0.0,  1.0, 0.35, false},
    {static_cast<std::uint32_t>(PianoParam::gain),         "Gain",         0.0,  1.0, 0.7,  false},
};

// ── Factory presets ──────────────────────────────────────────────────────────
//
// Each names only the parameters it cares about; anything it leaves out keeps
// the value the channel already had. The plain defaults are deliberately not
// among them — PresetLibrary synthesises "Default" from the tables above, so
// there is one copy of those numbers rather than two.

constexpr PresetValue samplerPluck[] = {
    {static_cast<std::uint32_t>(SamplerParam::attackSeconds), 0.001},
    {static_cast<std::uint32_t>(SamplerParam::decaySeconds), 0.25},
    {static_cast<std::uint32_t>(SamplerParam::sustainLevel), 0.0},
    {static_cast<std::uint32_t>(SamplerParam::releaseSeconds), 0.15},
};
constexpr PresetValue samplerPad[] = {
    {static_cast<std::uint32_t>(SamplerParam::attackSeconds), 0.6},
    {static_cast<std::uint32_t>(SamplerParam::decaySeconds), 1.0},
    {static_cast<std::uint32_t>(SamplerParam::sustainLevel), 0.8},
    {static_cast<std::uint32_t>(SamplerParam::releaseSeconds), 1.2},
    {static_cast<std::uint32_t>(SamplerParam::filterMode), 1.0},
    {static_cast<std::uint32_t>(SamplerParam::filterCutoffHz), 6000.0},
};
constexpr PresetValue samplerSweep[] = {
    {static_cast<std::uint32_t>(SamplerParam::filterMode), 1.0},
    {static_cast<std::uint32_t>(SamplerParam::filterCutoffHz), 1200.0},
    {static_cast<std::uint32_t>(SamplerParam::filterResonance), 3.0},
    {static_cast<std::uint32_t>(SamplerParam::lfoRateHz), 0.4},
    {static_cast<std::uint32_t>(SamplerParam::lfoToCutoff), 4.0},
};
constexpr PresetValue samplerVibrato[] = {
    {static_cast<std::uint32_t>(SamplerParam::lfoRateHz), 5.5},
    {static_cast<std::uint32_t>(SamplerParam::lfoToPitch), 0.4},
};

constexpr FactoryPreset samplerPresets[] = {
    {"Pluck",         samplerPluck,   std::size(samplerPluck)},
    {"Pad",           samplerPad,     std::size(samplerPad)},
    {"Filter Sweep",  samplerSweep,   std::size(samplerSweep)},
    {"Gentle Vibrato",samplerVibrato, std::size(samplerVibrato)},
};

constexpr PresetValue synthSawPad[] = {
    {static_cast<std::uint32_t>(SimpleSynthParam::waveform),
     static_cast<double>(SimpleSynth::Waveform::sawtooth)},
    {static_cast<std::uint32_t>(SimpleSynthParam::gain), 0.35},
    {static_cast<std::uint32_t>(SimpleSynthParam::attackSeconds), 0.45},
    {static_cast<std::uint32_t>(SimpleSynthParam::decaySeconds), 0.5},
    {static_cast<std::uint32_t>(SimpleSynthParam::sustainLevel), 0.7},
    {static_cast<std::uint32_t>(SimpleSynthParam::releaseSeconds), 0.9},
};
constexpr PresetValue synthSquareLead[] = {
    {static_cast<std::uint32_t>(SimpleSynthParam::waveform),
     static_cast<double>(SimpleSynth::Waveform::square)},
    {static_cast<std::uint32_t>(SimpleSynthParam::gain), 0.45},
    {static_cast<std::uint32_t>(SimpleSynthParam::attackSeconds), 0.005},
    {static_cast<std::uint32_t>(SimpleSynthParam::decaySeconds), 0.12},
    {static_cast<std::uint32_t>(SimpleSynthParam::sustainLevel), 0.75},
    {static_cast<std::uint32_t>(SimpleSynthParam::releaseSeconds), 0.12},
};
constexpr PresetValue synthSineBass[] = {
    {static_cast<std::uint32_t>(SimpleSynthParam::waveform),
     static_cast<double>(SimpleSynth::Waveform::sine)},
    {static_cast<std::uint32_t>(SimpleSynthParam::gain), 0.6},
    {static_cast<std::uint32_t>(SimpleSynthParam::attackSeconds), 0.002},
    {static_cast<std::uint32_t>(SimpleSynthParam::decaySeconds), 0.3},
    {static_cast<std::uint32_t>(SimpleSynthParam::sustainLevel), 0.5},
    {static_cast<std::uint32_t>(SimpleSynthParam::releaseSeconds), 0.08},
};

constexpr FactoryPreset simpleSynthPresets[] = {
    {"Saw Pad",      synthSawPad,     std::size(synthSawPad)},
    {"Square Lead",  synthSquareLead, std::size(synthSquareLead)},
    {"Sine Sub",     synthSineBass,   std::size(synthSineBass)},
};

constexpr PresetValue pianoConcert[] = {
    {static_cast<std::uint32_t>(PianoParam::model), static_cast<double>(PianoModel::grand)},
    {static_cast<std::uint32_t>(PianoParam::tone), 0.1},
    {static_cast<std::uint32_t>(PianoParam::decay), 1.2},
    {static_cast<std::uint32_t>(PianoParam::pedalTail), 0.45},
    {static_cast<std::uint32_t>(PianoParam::stereoSpread), 0.45},
};
constexpr PresetValue pianoPop[] = {
    {static_cast<std::uint32_t>(PianoParam::model), static_cast<double>(PianoModel::brightGrand)},
    {static_cast<std::uint32_t>(PianoParam::tone), 0.45},
    {static_cast<std::uint32_t>(PianoParam::hardness), 0.7},
    {static_cast<std::uint32_t>(PianoParam::hammerNoise), 0.45},
};
constexpr PresetValue pianoBallad[] = {
    {static_cast<std::uint32_t>(PianoParam::model), static_cast<double>(PianoModel::mellow)},
    {static_cast<std::uint32_t>(PianoParam::tone), -0.35},
    {static_cast<std::uint32_t>(PianoParam::hardness), 0.3},
    {static_cast<std::uint32_t>(PianoParam::decay), 1.4},
};
constexpr PresetValue pianoTine[] = {
    {static_cast<std::uint32_t>(PianoParam::model), static_cast<double>(PianoModel::electric)},
    {static_cast<std::uint32_t>(PianoParam::tone), 0.2},
    {static_cast<std::uint32_t>(PianoParam::hardness), 0.55},
    {static_cast<std::uint32_t>(PianoParam::stereoSpread), 0.2},
};
constexpr PresetValue pianoBar[] = {
    {static_cast<std::uint32_t>(PianoParam::model), static_cast<double>(PianoModel::upright)},
    {static_cast<std::uint32_t>(PianoParam::tone), -0.1},
    {static_cast<std::uint32_t>(PianoParam::stretchCents), 22.0},
    {static_cast<std::uint32_t>(PianoParam::hammerNoise), 0.5},
};

constexpr PresetValue wavetableSupersaw[] = {
    {static_cast<std::uint32_t>(WavetableParam::oscALevel), 0.45},
    {static_cast<std::uint32_t>(WavetableParam::oscADetune), -12.0},
    {static_cast<std::uint32_t>(WavetableParam::oscBLevel), 0.45},
    {static_cast<std::uint32_t>(WavetableParam::oscBDetune), 12.0},
    {static_cast<std::uint32_t>(WavetableParam::filterCutoffHz), 9000.0},
};
constexpr PresetValue wavetableFormant[] = {
    {static_cast<std::uint32_t>(WavetableParam::oscATable), 3.0},
    {static_cast<std::uint32_t>(WavetableParam::oscAPosition), 0.2},
    {static_cast<std::uint32_t>(WavetableParam::oscALevel), 0.6},
    {static_cast<std::uint32_t>(WavetableParam::lfo1RateHz), 0.3},
    {static_cast<std::uint32_t>(WavetableParam::lfo1ToPosition), 0.5},
};
constexpr PresetValue wavetableWobble[] = {
    {static_cast<std::uint32_t>(WavetableParam::oscATable), 1.0},
    {static_cast<std::uint32_t>(WavetableParam::oscAPosition), 0.4},
    {static_cast<std::uint32_t>(WavetableParam::subLevel), 0.5},
    {static_cast<std::uint32_t>(WavetableParam::filterMode), 1.0},
    {static_cast<std::uint32_t>(WavetableParam::filterCutoffHz), 400.0},
    {static_cast<std::uint32_t>(WavetableParam::filterResonance), 4.0},
    {static_cast<std::uint32_t>(WavetableParam::lfo1Shape), 1.0},
    {static_cast<std::uint32_t>(WavetableParam::lfo1RateHz), 4.0},
    {static_cast<std::uint32_t>(WavetableParam::lfo1ToCutoff), 3.0},
};
constexpr PresetValue wavetableBell[] = {
    {static_cast<std::uint32_t>(WavetableParam::oscATable), 4.0},
    {static_cast<std::uint32_t>(WavetableParam::oscAPosition), 0.75},
    {static_cast<std::uint32_t>(WavetableParam::ampDecay), 0.5},
    {static_cast<std::uint32_t>(WavetableParam::ampSustain), 0.0},
    {static_cast<std::uint32_t>(WavetableParam::ampRelease), 0.4},
    {static_cast<std::uint32_t>(WavetableParam::filterCutoffHz), 1500.0},
    {static_cast<std::uint32_t>(WavetableParam::modDecay), 0.3},
    {static_cast<std::uint32_t>(WavetableParam::modToCutoff), 4.0},
};
constexpr PresetValue wavetablePad[] = {
    {static_cast<std::uint32_t>(WavetableParam::oscATable), 2.0},
    {static_cast<std::uint32_t>(WavetableParam::oscALevel), 0.4},
    {static_cast<std::uint32_t>(WavetableParam::oscBTable), 2.0},
    {static_cast<std::uint32_t>(WavetableParam::oscBLevel), 0.3},
    {static_cast<std::uint32_t>(WavetableParam::ampAttack), 1.2},
    {static_cast<std::uint32_t>(WavetableParam::ampRelease), 1.5},
    {static_cast<std::uint32_t>(WavetableParam::filterCutoffHz), 6000.0},
    {static_cast<std::uint32_t>(WavetableParam::lfo2RateHz), 0.2},
    {static_cast<std::uint32_t>(WavetableParam::lfo2ToPosition), 0.35},
};

constexpr FactoryPreset wavetablePresets[] = {
    {"Detuned Saws", wavetableSupersaw, std::size(wavetableSupersaw)},
    {"Formant Sweep", wavetableFormant, std::size(wavetableFormant)},
    {"Wobble Bass",   wavetableWobble,  std::size(wavetableWobble)},
    {"Bell Pluck",    wavetableBell,    std::size(wavetableBell)},
    {"Slow Pad",      wavetablePad,     std::size(wavetablePad)},
};

constexpr FactoryPreset pianoPresets[] = {
    {"Concert Grand", pianoConcert, std::size(pianoConcert)},
    {"Pop Bright",    pianoPop,     std::size(pianoPop)},
    {"Ballad Mellow", pianoBallad,  std::size(pianoBallad)},
    {"Electric Tine", pianoTine,    std::size(pianoTine)},
    {"Bar Upright",   pianoBar,     std::size(pianoBar)},
};

} // namespace

const std::vector<BuiltinInstrumentInfo>& builtinInstruments()
{
    static const std::vector<BuiltinInstrumentInfo> infos = {
        {"incdaw.sampler", "Sampler", samplerParameters, 10,
         {samplerPresets, std::size(samplerPresets)}},
        {"incdaw.simplesynth", "Reference Synth", simpleSynthParameters, 6,
         {simpleSynthPresets, std::size(simpleSynthPresets)}},
        {"incdaw.piano", "INCDAW Piano", pianoParameters, 10,
         {pianoPresets, std::size(pianoPresets)}},
        {"incdaw.wavetable", "INCDAW Wavetable", wavetableParameters(),
         wavetableParameterCount(), {wavetablePresets, std::size(wavetablePresets)}},
    };

    return infos;
}

const BuiltinInstrumentInfo* findBuiltinInstrument(const std::string& uid)
{
    for (const BuiltinInstrumentInfo& info : builtinInstruments())
        if (uid == info.uid)
            return &info;

    return nullptr;
}

} // namespace incdaw::engine
