#pragma once

#include "engine/instrument/Instrument.h"
#include "plugins/PluginIdentifier.h"

#include <memory>

namespace incdaw::project {

/// Creates the instrument a channel asks for.
///
/// Lives in `project/` rather than `engine/` because it is a *lookup*: it turns
/// a persisted identifier into an object. The engine must not know that
/// identifiers exist, and the plugin host (Phase 13) will extend this function
/// rather than replace it — a hosted plugin and a built-in instrument are the
/// same thing to everything downstream.
///
/// Returns nullptr when the identifier names a plugin format INCDAW cannot host
/// yet. A channel whose instrument is missing renders silence and keeps its
/// place in the graph, so that the project still opens and nothing is lost on
/// the next save.
[[nodiscard]] std::unique_ptr<engine::Instrument> createInstrument(const plugins::PluginIdentifier& identifier);

/// The identifier a new channel is created with.
[[nodiscard]] plugins::PluginIdentifier defaultInstrumentIdentifier();

} // namespace incdaw::project
