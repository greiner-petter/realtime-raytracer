#ifndef MIRRORSHADER_H
#define MIRRORSHADER_H

#include "Shader.h"
#include "common/Types.h"

struct MirrorShader : public TypedShader<ShaderType::MirrorShader> {
    MirrorShader(const Vec3& color = Vec3(1.0f), float roughness = 0.0f) : throughput_roughness(Vec4(color, roughness)) {}

    virtual void* GetDataLayoutBeginPtr() override { return &throughput_roughness; }
    virtual size_t GetDataSize() const override { return sizeof(throughput_roughness); }

    Vec4 throughput_roughness;
};

#endif