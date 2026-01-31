#ifndef REFRACTIONSHADER_H
#define REFRACTIONSHADER_H

#include "Shader.h"
#include "common/Types.h"

struct RefractionShader : public TypedShader<ShaderType::RefractionShader> {
    RefractionShader(float indexInside, float indexOutside, float roughness = 0.0f, Vec3 color = Vec3(1.0f), float absorptionCoeff = 0.0f)
        : indexInside_Outside_Roughness(Vec4(indexInside, indexOutside, roughness, 0))
        , color_absorption(Vec4(color, absorptionCoeff)) {}

    virtual void* GetDataLayoutBeginPtr() override { return &indexInside_Outside_Roughness; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4) * 2; }

    Vec4 indexInside_Outside_Roughness;  // x=indexInside, y=indexOutside, z=roughness, w=unused
    Vec4 color_absorption;     // xyz=tint color, w=absorption coefficient
};

#endif
