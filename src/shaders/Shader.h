#ifndef SHADER_H
#define SHADER_H

enum class ShaderType : uint32_t {
    None = 0,
    FlatShader = 1,
    MirrorShader = 2,
    SimpleShadowShader = 3,
};

struct Shader {
    Shader(const ShaderType type) : type(type) {}
    virtual ~Shader() = default;

    virtual bool isTransparent() const { return false; }

    virtual void* GetDataLayoutBeginPtr() = 0;
    virtual size_t GetDataSize() const = 0;

    ShaderType type = ShaderType::None;
    int32_t index = -1;
};

template <ShaderType Type>
struct TypedShader : public Shader {
    explicit TypedShader() : Shader(Type) {}
};

#endif