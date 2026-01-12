#ifndef REFRACTIONSHADER_H
#define REFRACTIONSHADER_H

#include "Shader.h"
#include "common/Types.h"

struct RefractionShader : public TypedShader<ShaderType::RefractionShader> {
    RefractionShader(float indexInside, float indexOutside) : indexInside_Outside(Vec4(indexInside, indexOutside, 0, 0)) {}

    virtual void* GetDataLayoutBeginPtr() override { return &indexInside_Outside; }
    virtual size_t GetDataSize() const override { return sizeof(indexInside_Outside); }

    Vec4 indexInside_Outside;
};

#endif
