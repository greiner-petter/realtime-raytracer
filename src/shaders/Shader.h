#ifndef SHADER_H
#define SHADER_H

enum class ShaderType : uint32_t {
    None = 0,
    FlatShader = 1,
    RefractionShader = 2,
    MirrorShader = 3,
    SimpleShadowShader = 4,
    LambertShader = 5,
    PhongShader = 6,
    CookTorranceShader = 7,
    BRDFShader = 8,
    MaterialShader = 9,
};

struct Shader {
    Shader(const ShaderType type) : type(type) {}
    virtual ~Shader() = default;

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