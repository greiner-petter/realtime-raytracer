#ifndef COOKTORRANCESHADER_H
#define COOKTORRANCESHADER_H

#include "Shader.h"
#include "common/Types.h"

struct CookTorranceShader : public TypedShader<ShaderType::CookTorranceShader> {
    CookTorranceShader(Vec3 const &diffuseColor, Vec3 const &ctColor, float IOR, float roughness,
                     float diffuseCoefficient = PI / 2.0f, float ctCoefficient = PI / 2.0f)
    : diffuseColor_f0(Vec4(diffuseColor * diffuseCoefficient, IOR)),
      ctColor_m(Vec4(ctColor * ctCoefficient, roughness)) {}

    virtual void* GetDataLayoutBeginPtr() override { return &diffuseColor_f0; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4) * 2; }
    
    Vec4 diffuseColor_f0; // xyz = diffuseColor, w = f0
    Vec4 ctColor_m; // xyz = ctColor, w = m
};

#endif
