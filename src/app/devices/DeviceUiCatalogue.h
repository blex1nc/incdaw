#pragma once

#include "app/devices/DeviceUiSpec.h"

#include <string_view>
#include <vector>

namespace incdaw::app {

/// A family's entry point: appends a pointer to each of its specs (static
/// storage — a function-local static is the usual home). Called once from
/// DeviceUiCatalogue.cpp (docs/plugin-archive/00-CONTRACTS.md §3.3).
using DeviceUiRegistrar = void (*)(std::vector<const DeviceUiSpec*>&);

/// Every registered spec, in registration order.
[[nodiscard]] const std::vector<const DeviceUiSpec*>& deviceUiSpecs();

/// The spec for `uid`, or nullptr — which is legal: a device without a spec
/// gets the generic slider panel. Ship DSP first, spec second.
[[nodiscard]] const DeviceUiSpec* deviceUiSpec(std::string_view uid);

} // namespace incdaw::app
