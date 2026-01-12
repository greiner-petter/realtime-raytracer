#ifndef ARG_PARSE_H
#define ARG_PARSE_H

#include <unordered_map>
#include <functional>

using ArgFuncInput = std::pair<std::vector<std::string>, uint32_t&>;
using ArgFunc = std::function<void(ArgFuncInput)>;

template<typename T>
T NextArg(ArgFuncInput input);

struct ArgParse {
    static void ParseInput(int argc, char** argv);
    static void AddArgFunction(const std::string& input, const ArgFunc& func, const std::string& description);
    static void AddArgFunction(const std::initializer_list<std::string>& inputs, const ArgFunc& func, const std::string& description);

private:
    static void SetUpArgFunctions();
    
    inline static std::unordered_map<std::string, std::pair<ArgFunc, std::string>> s_ArgFunctions;


    static void PrintHelp(ArgFuncInput input);
};

#endif