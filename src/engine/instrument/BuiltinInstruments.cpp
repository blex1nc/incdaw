#include "engine/instrument/BuiltinInstruments.h"

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

} // namespace

const std::vector<BuiltinInstrumentInfo>& builtinInstruments()
{
    static const std::vector<BuiltinInstrumentInfo> infos = {
        {"incdaw.sampler", "Sampler", samplerParameters, 10},
        {"incdaw.simplesynth", "Reference Synth", simpleSynthParameters, 6},
    };

    return infos;
}

} // namespace incdaw::engine
