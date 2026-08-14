#include "plugins/PluginScan.h"

#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cstring>
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

    int pipeEnds[2];
    if (::pipe(pipeEnds) != 0) {
        outcome.detail = "pipe failed";
        return outcome;
    }

    // fork/exec rather than popen: plugin paths contain spaces and worse,
    // and an argv never meets a shell.
    const pid_t child = ::fork();

    if (child < 0) {
        ::close(pipeEnds[0]);
        ::close(pipeEnds[1]);
        outcome.detail = "fork failed";
        return outcome;
    }

    if (child == 0) {
        ::dup2(pipeEnds[1], STDOUT_FILENO);
        ::close(pipeEnds[0]);
        ::close(pipeEnds[1]);

        const std::string binary = scannerBinary.string();
        const std::string target = pluginPath.string();
        char* argv[] = {const_cast<char*>(binary.c_str()),
                        const_cast<char*>(target.c_str()), nullptr};

        ::execv(binary.c_str(), argv);
        ::_exit(127);   // exec failed; report as a plain failure, not a crash
    }

    ::close(pipeEnds[1]);

    std::string output;
    char buffer[4096];

    for (;;) {
        const ssize_t got = ::read(pipeEnds[0], buffer, sizeof(buffer));
        if (got <= 0)
            break;
        output.append(buffer, static_cast<std::size_t>(got));
    }

    ::close(pipeEnds[0]);

    int status = 0;
    ::waitpid(child, &status, 0);

    if (WIFSIGNALED(status)) {
        // The child died of the plugin's bug. This process did not.
        outcome.status = ScanOutcome::Status::crashed;
        outcome.detail = "scanner killed by signal " + std::to_string(WTERMSIG(status));
        return outcome;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        outcome.status = ScanOutcome::Status::failed;
        outcome.detail = "scanner exited with " + std::to_string(WEXITSTATUS(status));
        return outcome;
    }

    std::istringstream lines{output};
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
