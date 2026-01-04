#include "ShaderCompiler.h"

#include "common/Window.h"
#include "common/Subprocess.h"
#include "common/Log.h"

#include <fstream>
#include <filesystem>
#include <chrono>

static void CheckGlslangValidatorExists() {
    SubprocessResult result = RunCommand("glslangValidator --version");
    if (result.exitCode != 0) {
        RT_ERROR("glslangValidator not found. Please ensure it is installed and available in your PATH.");
        exit(1);
    }
}

std::filesystem::file_time_type GetFileModificationTime(const std::string& filePath) {
    try {
        return std::filesystem::last_write_time(filePath);
    }
    catch (const std::filesystem::filesystem_error& e) {
        RT_ERROR("Failed to get modification time for file {0}: {1}", filePath, e.what());
        return std::filesystem::file_time_type::min();
    }
}

std::filesystem::file_time_type GetShaderSourceModificationTime() {
    auto newest = std::filesystem::file_time_type::min();
    for (const auto& entry : std::filesystem::recursive_directory_iterator("ShaderCode")) {
        if (entry.is_regular_file() && entry.path().extension() == ".glsl") {
            auto modTime = GetFileModificationTime(entry.path().string());
            if (modTime > newest) newest = modTime;
        }
    }
    return newest;
}

std::filesystem::file_time_type GetCompiledShaderModificationTime() {
    try {
        if (!std::filesystem::exists("ShaderCache")) {
            return std::filesystem::file_time_type::min();
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        RT_ERROR("Failed to access ShaderCache directory: {0}", e.what());
        return std::filesystem::file_time_type::min();
    }

    auto oldest = std::filesystem::file_time_type::max();
    for (const auto& entry : std::filesystem::recursive_directory_iterator("ShaderCache")) {
        if (entry.is_regular_file() && entry.path().extension() == ".spv") {
            auto modTime = GetFileModificationTime(entry.path().string());
            if (modTime < oldest) oldest = modTime;
        }
    }
    return oldest;
}

void PreprocessShader(std::string& src) {
    // Simple include handling (no nested includes for simplicity)
    std::string includeDirective = "#include \"";
    size_t pos = 0;
    while ((pos = src.find(includeDirective, pos)) != std::string::npos) {
        size_t start = pos + includeDirective.length();
        size_t end = src.find("\"", start);
        if (end != std::string::npos) {
            std::string includePath = src.substr(start, end - start);
            std::ifstream includeFile("ShaderCode/include/" + includePath);
            if (includeFile.is_open()) {
                std::string includeContent((std::istreambuf_iterator<char>(includeFile)), std::istreambuf_iterator<char>());
                src.replace(pos, end - pos + 1, includeContent);
            } else {
                RT_ERROR("Failed to open included shader file: {0}", includePath);
                exit(1);
            }
        } else {
            break; // Malformed include
        }
    }
}

void ShaderCompiler::CompileShader(const std::string& shaderPath, const std::string& outputPath) {
    RT_INFO("Compiling shader: {0}", shaderPath);
    CheckGlslangValidatorExists();
    std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());
    SubprocessResult result = RunCommand("glslangValidator -V -IShaderCode/include " + shaderPath + " -o " + outputPath);
    if (result.exitCode != 0) {
        RT_ERROR("Failed to compile shader {0}: {1}", shaderPath, result.output);
        exit(1);
    } else {
        RT_INFO("Compiled shader {0} to {1}", shaderPath, outputPath);
    }
}

void ShaderCompiler::CompileAllShaders() {
    static bool stopLogSpam = false;
    if (GetCompiledShaderModificationTime() >= GetShaderSourceModificationTime()) {
        if (!stopLogSpam) {
            RT_INFO("All shaders are up to date. No compilation needed.");
        }
        stopLogSpam = true;
        return;
    }
    stopLogSpam = false;

    for (const auto& entry : std::filesystem::directory_iterator("ShaderCode")) {
        if (entry.is_regular_file() && entry.path().extension() == ".glsl") {
            std::string shaderPath = entry.path().string();
            std::string relativePath = std::filesystem::relative(entry.path(), "ShaderCode").string();
            std::string outputPath = "ShaderCache/" + relativePath + ".spv";

            // Ensure output directory exists
            std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());

            CompileShader(shaderPath, outputPath);
        }
    }

    // Mark as dirty to trigger any necessary reloads
    Window::GetInstance()->SetResizedFlag(true);
}

ShaderBinary::ShaderBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        RT_ERROR("Failed to open shader binary file: {0}", path);
        return;
    }
    m_SizeInBytes = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    m_Data = new byte[m_SizeInBytes];
    file.read(reinterpret_cast<char*>(m_Data), m_SizeInBytes);
    file.close();
}

ShaderBinary::~ShaderBinary() {
    delete[] m_Data;
}