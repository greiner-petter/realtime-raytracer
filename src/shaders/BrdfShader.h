#ifndef BRDFSHADER_H
#define BRDFSHADER_H

#include "shaders/Shader.h"
#include "vulkan/Brdf.h"
#include "common/Types.h"

struct BrdfShader : public TypedShader<ShaderType::BrdfShader> {
    BrdfShader(Vec3 const& colorScale = Vec3(1.0f))
        : scaleIndex(Vec4(colorScale, static_cast<float>(NULL_BRDF))) {}

    void setBrdf(BrdfID brdfId) { scaleIndex.w = static_cast<float>(brdfId); }
    void setColorScale(const Vec3& scale) { scaleIndex.x = scale.x; scaleIndex.y = scale.y; scaleIndex.z = scale.z; }

    BrdfID getBrdf() const { return static_cast<BrdfID>(scaleIndex.w); }

    virtual void* GetDataLayoutBeginPtr() override { return &scaleIndex; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4); }

    Vec4 scaleIndex;  // xyz = color scale, w = BRDF ID (index into BRDF buffer)
};

#endif
