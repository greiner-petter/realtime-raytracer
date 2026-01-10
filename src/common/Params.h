#ifndef PARAMS_H
#define PARAMS_H

#include "common/Types.h"

struct Params {
    static bool IsInteractiveMode() { return s_InteractiveMode; }
    static uint32_t GetWidth() { return s_Width; }
    static uint32_t GetHeight() { return s_Height; }
    static uint32_t GetSampleCount() { return s_Samples; }
    static std::string GetResultImageName() { return s_ResultImageName; }
    static std::string GetInputSceneFilename() { return s_InputScene; }

    inline static uint32_t s_Width = 3840;
    inline static uint32_t s_Height = 2160;
    inline static bool s_InteractiveMode = true;
    inline static uint32_t s_Samples = 1024;
    inline static std::string s_ResultImageName = "result.png";
    inline static std::string s_InputScene = "";

    constexpr static bool ENABLE_SHADER_HOT_RELOAD = true;
};

#endif