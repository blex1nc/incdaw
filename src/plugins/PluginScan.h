#pragma once

#include "plugins/clap/ClapLibrary.h"

#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::plugins {

/// The result of scanning one library in a child process.
///
/// `crashed` is the outcome this mechanism exists for: the child died on the
/// plugin's behalf and this process never felt it. A crashed or failed path
/// belongs on the blacklist; only `ok` libraries are ever loaded in-process
/// (docs/PLUGIN_HOST.md §3).
struct ScanOutcome {
    enum class Status { ok, failed, crashed };

    Status                      status = Status::failed;
    std::vector<ClapDescriptor> plugins;
    std::string                 detail;
};

/// Runs `scannerBinary pluginPath` as a child process and parses its report.
/// Never loads the plugin here — that is the entire point.
[[nodiscard]] ScanOutcome scanOutOfProcess(const std::filesystem::path& scannerBinary,
                                           const std::filesystem::path& pluginPath);

} // namespace incdaw::plugins
