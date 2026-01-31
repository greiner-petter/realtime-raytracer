#ifndef EMISSIVESHADER_H
#define EMISSIVESHADER_H

#include "Shader.h"
#include "common/Types.h"

struct EmissiveShader : public TypedShader<ShaderType::EmissiveShader> {
    EmissiveShader(const Vec3& color, float intensity = 1.0f)
        : color_intensity(Vec4(color, intensity)) {}

    virtual void* GetDataLayoutBeginPtr() override { return &color_intensity; }
    virtual size_t GetDataSize() const override { return sizeof(color_intensity); }

    Vec4 color_intensity; // xyz = color, w = intensity
};

#endif
