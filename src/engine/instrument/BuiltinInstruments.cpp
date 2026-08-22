#include "engine/instrument/BuiltinInstruments.h"

#include "engine/instrument/PianoInstrument.h"
#include "engine/instrument/Sampler.h"
#include "engine/instrument/SimpleSynth.h"

namespace incdaw::engine {

namespace {

using dsp::EffectParameter;

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

} // namespace

const std::vector<BuiltinInstrumentInfo>& builtinInstruments()
{
    static const std::vector<BuiltinInstrumentInfo> infos = {
        {"incdaw.sampler", "Sampler", samplerParameters, 10},
        {"incdaw.simplesynth", "Reference Synth", simpleSynthParameters, 6},
        {"incdaw.piano", "INCDAW Piano", pianoParameters, 10},
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
