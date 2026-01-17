#ifndef SIMPLE_TEXTURE_SHADER_H
#define SIMPLE_TEXTURE_SHADER_H

#include "Shader.h"
#include "common/Types.h"
#include "vulkan/Texture.h"

struct SimpleTextureShader : public TypedShader<ShaderType::SimpleTextureShader> {
    SimpleTextureShader() : color_texture(Vec4(1, 1, 1, 0)) {}

    SimpleTextureShader(TextureID texture) {
        color_texture = Vec4(Vec3(1.0f, 1.0f, 1.0f), texture);
    }
    SimpleTextureShader(const Vec3& color, TextureID texture) {
        color_texture = Vec4(color, texture);
    }

    virtual void* GetDataLayoutBeginPtr() override { return &color_texture; }
    virtual size_t GetDataSize() const override { return sizeof(color_texture); }

    Vec4 color_texture; // xyz = rgb, w = texture_id
};

#endif