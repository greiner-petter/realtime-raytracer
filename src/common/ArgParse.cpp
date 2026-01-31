#include "ArgParse.h"

#include <string>
#include <vector>
#include <iostream>
#include "Params.h"
#include "Log.h"

uint32_t to_uint32(const std::string& s) {
    size_t pos = 0;
    unsigned long value = std::stoul(s, &pos, 10);

    // Reject trailing characters (e.g. "123abc")
    if (pos != s.size()) {
        throw std::invalid_argument("Invalid uint32: trailing characters");
    }

    // Range check
    if (value > std::numeric_limits<uint32_t>::max()) {
        throw std::out_of_range("Value out of range for uint32");
    }

    return static_cast<uint32_t>(value);
}

template<>
std::string NextArg<std::string>(ArgFuncInput input) {
    RT_ASSERT(input.second + 1 < int(input.first.size()), "missing command line argument");
    return input.first[++input.second];
}

template<>
uint32_t NextArg<uint32_t>(ArgFuncInput input) {
    RT_ASSERT(input.second + 1 < int(input.first.size()), "missing command line argument");
    return to_uint32(input.first[++input.second]);
}

void ArgParse::PrintHelp(ArgFuncInput input) {
    std::cout << "Usage: tracey_rt <arguments>" << std::endl;
    std::cout << " * using no arguments runs tracey_rt in interactive mode" << std::endl;
    std::unordered_map<std::string, std::vector<std::string>> commands;
    for (auto& [cmd, value] : s_ArgFunctions) {
        auto [func, desc] = value;
        if (commands.find(desc) == commands.end()) {
            commands[desc] = { };
        }
        commands[desc].push_back(cmd);
    }

    std::vector<std::pair<std::string, std::string>> lines;
    for (auto& [desc, cmds] : commands) {
        std::string left;
        for (int i = 0; i < cmds.size(); i++) {
            left += cmds[i];
            if (i != cmds.size() - 1) {
                left += ", ";
            }
        }
        lines.push_back({left, desc});
    }

    // get maximum length from all lines
    size_t max_length = 0;
    for (auto& [left, right] : lines) {
        max_length = std::max(max_length, left.size());
    }

    // actually print each line
    for (auto& [left, right] : lines) {
        std::cout << left;
        for (size_t i = 0; i < max_length - left.size() + 3; i++) {
            std::cout << " ";
        }
        std::cout << right << std::endl;
    }

    exit(EXIT_SUCCESS);
}

static void PrintVersion(ArgFuncInput input) {
    std::cout << "tracey_rt - Vulkan GPU Raytracer - Version 1.0" << std::endl;
    exit(EXIT_SUCCESS);
}

void ArgParse::ParseInput(int argc, char** argv) {
    ArgParse::SetUpArgFunctions();

    std::vector<std::string> args;

    for (uint32_t i = 1; i < argc; i++) {
        args.push_back(std::string(argv[i]));
    }

    for (uint32_t i = 0; i < args.size(); i++) {
        if (s_ArgFunctions.find(args[i]) == s_ArgFunctions.end()) {
            RT_ERROR("Cannot parse command line argument '{0}' use --help for more information", args[i]);
            exit(EXIT_FAILURE);
        }

        s_ArgFunctions.at(args[i]).first({ args, i });
    }
}


void ArgParse::AddArgFunction(const std::string& input, const ArgFunc& func, const std::string& description) {
    s_ArgFunctions[input] = { func, description };
}
void ArgParse::AddArgFunction(const std::initializer_list<std::string>& inputs, const ArgFunc& func, const std::string& description) {
    for (auto& it : inputs) {
        AddArgFunction(it, func, description);
    }
}

void ArgParse::SetUpArgFunctions() {
    AddArgFunction({"-h", "--help"}, PrintHelp, "Display this text");
    AddArgFunction({"-i", "--input"}, [](ArgFuncInput input) {
        Params::s_InputScene = NextArg<std::string>(input); 
    }, "<filename> set the input scene");
    AddArgFunction({"-o", "--output"}, [](ArgFuncInput input) {
        Params::s_ResultImageName = NextArg<std::string>(input); 
    }, "<filename> set the ResultImageName for the output");
    AddArgFunction({"-d", "--dim", "--dimension", "--size"}, [](ArgFuncInput input) {
        Params::s_Width = NextArg<uint32_t>(input); 
        Params::s_Height = NextArg<uint32_t>(input); 
        Params::s_InteractiveMode = false;
    }, "<width> <height> set the image dimension");
    AddArgFunction({"-s", "--samples"}, [](ArgFuncInput input) {
        Params::s_Samples = NextArg<uint32_t>(input); 
    }, "<samples> set the image samples");
    AddArgFunction("--non-interactive", [](ArgFuncInput input) { Params::s_InteractiveMode = false; }, "Run tracey_rt in non-interactive mode explicitly");
    AddArgFunction("--version", PrintVersion, "Display the version");
}
