#ifndef SHADER_COMPILER_H
#define SHADER_COMPILER_H

#include "common/Types.h"
#include <string>
#include <memory>

struct ShaderBinary {
    ShaderBinary(const std::string& path);
    ~ShaderBinary();

    size_t GetSizeInBytes() const { return m_SizeInBytes; }
    byte* GetData() const { return m_Data; }
private:
    size_t m_SizeInBytes = 0;
    byte* m_Data = nullptr;
};

class ShaderCompiler {
public:
    static void CompileShader(const std::string& shaderPath, const std::string& outputPath);
    static void CompileAllShaders();
};

#endif