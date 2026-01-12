#ifndef FLATSHADER_H
#define FLATSHADER_H

#include "Shader.h"
#include "common/Types.h"

struct FlatShader : public TypedShader<ShaderType::FlatShader> {
    FlatShader() : color(Vec4(1, 1, 1, 0)) {}

    FlatShader(const Vec3& color) : color(Vec4(color, 0)) {}

    virtual void* GetDataLayoutBeginPtr() override { return &color; }
    virtual size_t GetDataSize() const override { return sizeof(color); }

    Vec4 color; // xyz = rgb
};

#endif