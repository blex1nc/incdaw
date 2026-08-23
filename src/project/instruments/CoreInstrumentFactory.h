#pragma once

#include "project/InstrumentFactory.h"

namespace incdaw::project {

/// The three instruments INCDAW shipped with before the archive: the
/// reference synth, the piano and the sampler. The sampler is the one that
/// needs the resolver — its zones are the project's audio assets, decoded
/// (or streamed) at compile time.
void registerCoreInstruments(std::vector<BuiltinInstrumentEntry>& rows);

} // namespace incdaw::project
