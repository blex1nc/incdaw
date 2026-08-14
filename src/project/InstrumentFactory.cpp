#include "project/InstrumentFactory.h"

#include "engine/instrument/SimpleSynth.h"

namespace incdaw::project {

std::unique_ptr<engine::Instrument> createInstrument(const plugins::PluginIdentifier& identifier)
{
    if (identifier.format != plugins::Format::builtin)
        return nullptr;   // hosted formats arrive with Phase 13

    if (identifier.uid == plugins::builtinSynthUid)
        return std::make_unique<engine::SimpleSynth>();

    return nullptr;
}

plugins::PluginIdentifier defaultInstrumentIdentifier()
{
    return {plugins::Format::builtin, plugins::builtinSynthUid};
}

} // namespace incdaw::project
