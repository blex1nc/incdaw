#include "plugins/PluginIdentifier.h"

namespace incdaw::plugins {

const char* formatName(Format format) noexcept
{
    switch (format) {
        case Format::clap:      return "clap";
        case Format::audioUnit: return "au";
        case Format::vst3:      return "vst3";
    }
    return "unknown";
}

std::string PluginIdentifier::toString() const
{
    return std::string{formatName(format)} + ':' + uid;
}

bool PluginIdentifier::fromString(const std::string& text, PluginIdentifier& out)
{
    const auto separator = text.find(':');
    if (separator == std::string::npos || separator + 1 >= text.size())
        return false;

    const std::string formatText = text.substr(0, separator);

    if      (formatText == "clap") out.format = Format::clap;
    else if (formatText == "au")   out.format = Format::audioUnit;
    else if (formatText == "vst3") out.format = Format::vst3;
    else                           return false;

    // Everything after the first colon is the uid: AU and VST3 ids can
    // themselves contain colons, so splitting on the last one would corrupt them.
    out.uid = text.substr(separator + 1);
    return true;
}

} // namespace incdaw::plugins
