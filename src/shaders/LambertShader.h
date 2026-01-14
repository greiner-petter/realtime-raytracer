#ifndef LAMBERTSHADER_H
#define LAMBERTSHADER_H

#include "Shader.h"
#include "common/Types.h"

struct LambertShader : public TypedShader<ShaderType::LambertShader> {
    LambertShader(Vec3 const &diffuseColor) : diffuseColor(Vec4(diffuseColor, 0)) {}

    virtual void* GetDataLayoutBeginPtr() override { return &diffuseColor; }
    virtual size_t GetDataSize() const override { return sizeof(diffuseColor); }
    
    Vec4 diffuseColor;
};

#endif
