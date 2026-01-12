#include "Subprocess.h"
#include <cstdio>
#include <array>

#if defined(_WIN32)
    #define POPEN  _popen
    #define PCLOSE _pclose
#else
    #define POPEN  popen
    #define PCLOSE pclose
#endif

SubprocessResult RunCommand(const std::string& command) {
    SubprocessResult result;

    // Redirect stderr to stdout so we capture everything
#if defined(_WIN32)
    std::string cmd = command + " 2>&1";
#else
    std::string cmd = command + " 2>&1";
#endif

    FILE* pipe = POPEN(cmd.c_str(), "r");
    if (!pipe) {
        result.output = "Failed to start subprocess";
        return result;
    }

    std::array<char, 4096> buffer{};
    while (fgets(buffer.data(), buffer.size(), pipe)) {
        result.output += buffer.data();
    }

    int rc = PCLOSE(pipe);

#if defined(_WIN32)
    result.exitCode = rc;
#else
    if (WIFEXITED(rc))
        result.exitCode = WEXITSTATUS(rc);
    else
        result.exitCode = rc;
#endif

    return result;
}
