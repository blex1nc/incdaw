#include "platform/ChildProcess.h"

#include <sys/wait.h>
#include <unistd.h>

namespace incdaw::platform {

ChildResult runChildProcess(const std::filesystem::path& binary,
                            const std::vector<std::string>& arguments)
{
    ChildResult result;

    int pipeEnds[2];
    if (::pipe(pipeEnds) != 0)
        return result;

    const pid_t child = ::fork();

    if (child < 0) {
        ::close(pipeEnds[0]);
        ::close(pipeEnds[1]);
        return result;
    }

    if (child == 0) {
        ::dup2(pipeEnds[1], STDOUT_FILENO);
        ::close(pipeEnds[0]);
        ::close(pipeEnds[1]);

        const std::string program = binary.string();

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(program.c_str()));
        for (const std::string& argument : arguments)
            argv.push_back(const_cast<char*>(argument.c_str()));
        argv.push_back(nullptr);

        ::execv(program.c_str(), argv.data());
        ::_exit(127);   // exec failed: a plain failure, not a crash
    }

    ::close(pipeEnds[1]);

    char buffer[4096];
    for (;;) {
        const ssize_t got = ::read(pipeEnds[0], buffer, sizeof(buffer));
        if (got <= 0)
            break;
        result.output.append(buffer, static_cast<std::size_t>(got));
    }

    ::close(pipeEnds[0]);

    int status = 0;
    ::waitpid(child, &status, 0);

    if (WIFSIGNALED(status)) {
        result.end  = ChildResult::End::signalled;
        result.code = WTERMSIG(status);
    } else if (WIFEXITED(status)) {
        result.end  = ChildResult::End::exited;
        result.code = WEXITSTATUS(status);
    }

    return result;
}

} // namespace incdaw::platform
