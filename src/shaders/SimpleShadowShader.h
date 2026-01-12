#ifndef SIMPLESHADOWSHADER_H
#define SIMPLESHADOWSHADER_H

#include "Shader.h"
#include "common/Types.h"

struct SimpleShadowShader : public TypedShader<ShaderType::SimpleShadowShader> {
    SimpleShadowShader(Vec3 const &objectColor) : objectColor(Vec4(objectColor, 0)) {}

    virtual void* GetDataLayoutBeginPtr() override { return &objectColor; }
    virtual size_t GetDataSize() const override { return sizeof(objectColor); }
    
    Vec4 objectColor;
};

#endif
