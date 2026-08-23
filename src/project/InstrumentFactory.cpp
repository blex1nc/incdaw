#include "project/InstrumentFactory.h"

#include "engine/instrument/SimpleSynth.h"
#include "project/instruments/CoreInstrumentFactory.h"
// <<< incdaw:registrars:instruments:include — one line per family
// >>> incdaw:registrars:instruments:include

namespace incdaw::project {

InstrumentFactory defaultInstrumentFactory()
{
    return [](const Channel&) -> std::unique_ptr<engine::Instrument> {
        return std::make_unique<engine::SimpleSynth>();
    };
}

const std::vector<BuiltinInstrumentEntry>& builtinInstrumentEntries()
{
    static const std::vector<BuiltinInstrumentEntry> entries = [] {
        std::vector<BuiltinInstrumentEntry> rows;

        // <<< incdaw:registrars:instruments — one line per family
        registerCoreInstruments(rows);
        // >>> incdaw:registrars:instruments

        return rows;
    }();

    return entries;
}

const BuiltinInstrumentEntry* findBuiltinInstrumentEntry(std::string_view uid)
{
    for (const BuiltinInstrumentEntry& entry : builtinInstrumentEntries())
        if (uid == entry.uid)
            return &entry;

    return nullptr;
}

std::unique_ptr<engine::Instrument> makeBuiltinInstrument(const InstrumentBuildContext& context)
{
    if (const BuiltinInstrumentEntry* entry =
            findBuiltinInstrumentEntry(context.channel.instrument.uid))
        return entry->make(context);

    return nullptr;
}

} // namespace incdaw::project
