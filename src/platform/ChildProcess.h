#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::platform {

/// One child process, run to completion, stdout captured.
///
/// fork/exec rather than popen: the arguments never meet a shell, which is
/// what makes paths with spaces (and worse) safe. `signalled` is reported
/// distinctly because the plugin scanner's whole reason to exist is telling
/// "it crashed" apart from "it declined".
struct ChildResult {
    enum class End { exited, signalled, failed };

    End         end    = End::failed;
    int         code   = 0;       ///< exit status, or the signal number
    std::string output;           ///< everything the child wrote to stdout
};

[[nodiscard]] ChildResult runChildProcess(const std::filesystem::path& binary,
                                          const std::vector<std::string>& arguments);

} // namespace incdaw::platform
