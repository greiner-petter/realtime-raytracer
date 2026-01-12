#ifndef MIRRORSHADER_H
#define MIRRORSHADER_H

#include "Shader.h"
#include "common/Types.h"

struct MirrorShader : public TypedShader<ShaderType::MirrorShader> {
    MirrorShader() : throughput(Vec4(1, 1, 1, 0)) {}

    MirrorShader(const Vec3& color) : throughput(Vec4(color, 0)) {}

    virtual void* GetDataLayoutBeginPtr() override { return &throughput; }
    virtual size_t GetDataSize() const override { return sizeof(throughput); }

    Vec4 throughput;
};

#endif