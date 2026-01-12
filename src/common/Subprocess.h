#ifndef SUBPROCESS_H
#define SUBPROCESS_H

#include <string>

struct SubprocessResult {
    int exitCode = -1;
    std::string output;
};

// Runs a command via the system shell
// Example: RunCommand("ls -la");
SubprocessResult RunCommand(const std::string& command);

#endif