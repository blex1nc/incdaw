#include "project/ParameterRegistry.h"

namespace incdaw::project {

ParameterRegistry ParameterRegistry::withBuiltins()
{
    ParameterRegistry registry;

    // Volume maps through the same cubic law the mixer's fader uses, so an
    // automation ramp and a hand on the fader travel identically. 0.63 lands on
    // unity: (0.63^3)*4 ≈ 1.
    registry.registerParameter("volume", [](engine::dsp::MixerStripNode& strip, float value) {
        strip.setGain(value * value * value * 4.0f);
    });

    registry.registerParameter("pan", [](engine::dsp::MixerStripNode& strip, float value) {
        strip.setPan(static_cast<double>(value) * 2.0 - 1.0);
    });

    return registry;
}

void ParameterRegistry::registerParameter(std::string key, StripApplier apply)
{
    for (Entry& entry : entries_) {
        if (entry.key == key) {
            entry.apply = std::move(apply);
            return;
        }
    }

    entries_.push_back({std::move(key), std::move(apply)});
}

const ParameterRegistry::Entry* ParameterRegistry::find(const std::string& key) const noexcept
{
    for (const Entry& entry : entries_)
        if (entry.key == key)
            return &entry;

    return nullptr;
}

} // namespace incdaw::project
