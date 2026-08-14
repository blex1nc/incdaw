#include "plugins/PluginScan.h"

#include "platform/ChildProcess.h"

#include <array>
#include <sstream>

namespace incdaw::plugins {
namespace {

/// Splits one scanner report line: PLUGIN\tid\tname\tvendor\tversion.
bool parseLine(const std::string& line, ClapDescriptor& out)
{
    std::array<std::string, 5> fields;
    std::size_t field = 0;
    std::size_t start = 0;

    for (std::size_t index = 0; index <= line.size() && field < fields.size(); ++index) {
        if (index == line.size() || line[index] == '\t') {
            fields[field++] = line.substr(start, index - start);
            start = index + 1;
        }
    }

    if (field < 2 || fields[0] != "PLUGIN" || fields[1].empty())
        return false;

    out.id      = fields[1];
    out.name    = fields[2];
    out.vendor  = fields[3];
    out.version = fields[4];
    return true;
}

} // namespace

ScanOutcome scanOutOfProcess(const std::filesystem::path& scannerBinary,
                             const std::filesystem::path& pluginPath)
{
    ScanOutcome outcome;

    const platform::ChildResult child =
        platform::runChildProcess(scannerBinary, {pluginPath.string()});

    if (child.end == platform::ChildResult::End::signalled) {
        // The child died of the plugin's bug. This process did not.
        outcome.status = ScanOutcome::Status::crashed;
        outcome.detail = "scanner killed by signal " + std::to_string(child.code);
        return outcome;
    }

    if (child.end != platform::ChildResult::End::exited || child.code != 0) {
        outcome.status = ScanOutcome::Status::failed;
        outcome.detail = "scanner exited with " + std::to_string(child.code);
        return outcome;
    }

    std::istringstream lines{child.output};
    std::string        line;

    while (std::getline(lines, line)) {
        ClapDescriptor descriptor;
        if (parseLine(line, descriptor))
            outcome.plugins.push_back(std::move(descriptor));
    }

    outcome.status = ScanOutcome::Status::ok;
    return outcome;
}

} // namespace incdaw::plugins
