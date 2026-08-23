#include "app/devices/DeviceUiCatalogue.h"

// <<< incdaw:registrars:panels:include — one line per family (Agents 2 and 3)
// >>> incdaw:registrars:panels:include

namespace incdaw::app {

const std::vector<const DeviceUiSpec*>& deviceUiSpecs()
{
    static const std::vector<const DeviceUiSpec*> specs = [] {
        std::vector<const DeviceUiSpec*> rows;

        // <<< incdaw:registrars:panels — one line per family (Agents 2 and 3)
        // >>> incdaw:registrars:panels

        return rows;
    }();

    return specs;
}

const DeviceUiSpec* deviceUiSpec(std::string_view uid)
{
    for (const DeviceUiSpec* spec : deviceUiSpecs())
        if (spec != nullptr && uid == spec->uid)
            return spec;

    return nullptr;
}

} // namespace incdaw::app
