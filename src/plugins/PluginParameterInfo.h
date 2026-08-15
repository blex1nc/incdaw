#pragma once

#include <cstdint>
#include <string>

namespace incdaw::plugins {

/// One discovered plugin parameter, in format-agnostic terms.
///
/// This header stays free of any plugin-format ABI on purpose: project/ reads
/// it to register parameters, and docs/DECISIONS.md D-028 keeps CLAP headers
/// out of that layer. Values are PLAIN — the parameter's own units and range,
/// which is what CLAP events carry; mapping from the automation system's
/// normalised 0..1 happens in the registry's applier.
struct PluginParameterInfo {
    std::uint32_t id           = 0;
    std::string   name;
    double        minValue     = 0.0;
    double        maxValue     = 1.0;
    double        defaultValue = 0.0;
    bool          stepped      = false;
};

} // namespace incdaw::plugins
