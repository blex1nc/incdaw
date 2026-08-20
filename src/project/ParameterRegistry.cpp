#include "project/ParameterRegistry.h"

#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/instrument/BuiltinInstruments.h"

#include <cmath>

namespace incdaw::project {

ParameterRegistry ParameterRegistry::withBuiltins()
{
    ParameterRegistry registry;

    // Volume maps through the same cubic law the mixer's fader uses, so an
    // automation ramp and a hand on the fader travel identically. 0.63 lands on
    // unity: (0.63^3)*4 ≈ 1.
    registry.registerParameter("volume", StripApplier{[](engine::dsp::MixerStripNode& strip,
                                                         float value) {
        strip.setGain(value * value * value * 4.0f);
    }});

    registry.registerParameter("pan", StripApplier{[](engine::dsp::MixerStripNode& strip,
                                                      float value) {
        strip.setPan(static_cast<double>(value) * 2.0 - 1.0);
    }});

    registry.registerParameter("stereoSeparation",
                               StripApplier{[](engine::dsp::MixerStripNode& strip, float value) {
                                   strip.setStereoSeparation(static_cast<double>(value) * 2.0
                                                             - 1.0);
                               }});

    return registry;
}

std::string ParameterRegistry::pluginParameterKey(const std::string& pluginUid,
                                                  std::uint32_t      parameterId)
{
    return "plugin:" + pluginUid + ":" + std::to_string(parameterId);
}

void ParameterRegistry::registerParameter(std::string key, Applier apply)
{
    for (Entry& entry : entries_) {
        if (entry.key == key) {
            entry.apply = std::move(apply);
            return;
        }
    }

    entries_.push_back({std::move(key), std::move(apply)});
}

void ParameterRegistry::registerPluginParameters(
    const std::string& pluginUid, const std::vector<plugins::PluginParameterInfo>& parameters)
{
    for (const plugins::PluginParameterInfo& parameter : parameters) {
        // Captured by value, field by field: the applier outlives this call by
        // as long as any graph compiled against the registry.
        const std::uint32_t id      = parameter.id;
        const double        min     = parameter.minValue;
        const double        range   = parameter.maxValue - parameter.minValue;
        const bool          stepped = parameter.stepped;

        registerParameter(pluginParameterKey(pluginUid, id),
                          SinkApplier{[id, min, range, stepped](engine::ParameterSink& sink,
                                                                float value) {
                              double plain = min + static_cast<double>(value) * range;
                              if (stepped)
                                  plain = std::round(plain);

                              sink.setParameter(id, plain);
                          }});
    }
}

namespace {

std::vector<plugins::PluginParameterInfo>
convertParameters(const engine::dsp::EffectParameter* parameters, std::size_t count)
{
    std::vector<plugins::PluginParameterInfo> converted;
    converted.reserve(count);

    for (std::size_t index = 0; index < count; ++index) {
        const engine::dsp::EffectParameter& parameter = parameters[index];

        plugins::PluginParameterInfo entry;
        entry.id           = parameter.id;
        entry.name         = parameter.name;
        entry.minValue     = parameter.minValue;
        entry.maxValue     = parameter.maxValue;
        entry.defaultValue = parameter.defaultValue;
        entry.stepped      = parameter.stepped;
        converted.push_back(std::move(entry));
    }

    return converted;
}

} // namespace

void ParameterRegistry::registerBuiltinEffects()
{
    for (const engine::dsp::BuiltinEffectInfo& info : engine::dsp::builtinEffects())
        registerPluginParameters(info.uid,
                                 convertParameters(info.parameters, info.parameterCount));
}

void ParameterRegistry::registerBuiltinInstruments()
{
    for (const engine::BuiltinInstrumentInfo& info : engine::builtinInstruments())
        registerPluginParameters(info.uid,
                                 convertParameters(info.parameters, info.parameterCount));
}

const ParameterRegistry::Entry* ParameterRegistry::find(const std::string& key) const noexcept
{
    for (const Entry& entry : entries_)
        if (entry.key == key)
            return &entry;

    return nullptr;
}

} // namespace incdaw::project
