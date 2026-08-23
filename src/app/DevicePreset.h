#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace incdaw::app {

/// One stored setting of one device — the shape only; banks, browsing and
/// the commands that act on it arrive with the preset system (Agent 1 Wave
/// 3, gap A5). A preset is the device's own StateIO blob under a name, so
/// loading one is exactly what loading a project's insert state already is.
struct DevicePreset {
    std::string deviceUid;                 ///< "incdaw.eq" — which device the blob belongs to
    std::string name;
    std::string author;
    std::vector<std::string> tags;         ///< free-form: "bass", "vocal", "lofi"
    std::vector<std::uint8_t> state;       ///< the device's StateIO::saveState output
};

} // namespace incdaw::app
