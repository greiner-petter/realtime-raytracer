#ifndef PHONGSHADER_H
#define PHONGSHADER_H

#include "Shader.h"
#include "common/Types.h"

struct PhongShader : public TypedShader<ShaderType::PhongShader> {
    PhongShader(Vec3 const &diffuseColor, float diffuseCoefficient, Vec3 const &specularColor,
                         float specularCoefficient, float shininessExponent)
    : diffuseColor_Coefficient(Vec4(diffuseColor, diffuseCoefficient)), 
      specularColor_Coefficient(Vec4(specularColor, specularCoefficient)),
      shininessExponent(Vec4(shininessExponent, 0, 0, 0)) {}

    virtual void* GetDataLayoutBeginPtr() override { return &diffuseColor_Coefficient; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4) * 3; }
    
    Vec4 diffuseColor_Coefficient;
    Vec4 specularColor_Coefficient;
    Vec4 shininessExponent;
};

#endif
