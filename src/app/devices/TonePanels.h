#pragma once

#include "app/devices/DeviceUiCatalogue.h"

namespace incdaw::app {

/// The Tone insert's face: three bipolar gain knobs over the EQ's response
/// curve, with the frequencies and Q folded away under "Advanced". The one
/// spec Agent 1 owns — it is the existence proof the vocabulary was sized
/// against (docs/plugin-archive/AGENT-1-FRAMEWORK.md, Wave 1).
void registerTonePanels(std::vector<const DeviceUiSpec*>& specs);

} // namespace incdaw::app
