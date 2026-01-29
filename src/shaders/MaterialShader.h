#ifndef MATERIALSHADER_H
#define MATERIALSHADER_H

#include "vulkan/Texture.h"
#include "shaders/Shader.h"
#include <bit>

struct MaterialShader : public TypedShader<ShaderType::MaterialShader>  {
    // Constructor
    MaterialShader() : alphaMap_opacity(Vec4(NULL_TEXTURE, 1, 0, 0)),
                       normalMap_coefficient(Vec4(NULL_TEXTURE, 1, 0, 0)),
                       diffuseMap_coefficient(Vec4(NULL_TEXTURE, 0.5f, 0, 0)),
                       reflectionMap_reflectance(Vec4(NULL_TEXTURE, 0, 0, 0)),
                       specularMap_coefficient_exponent(Vec4(NULL_TEXTURE, 0.5f, 8, 0)) {}

    // Set
    void setAlphaMap(TextureID const alphaMap) { alphaMap_opacity.x = float(alphaMap); }
    void setOpacity(float opacity) { alphaMap_opacity.y = opacity; }
    void setNormalMap(TextureID const normalMap) { normalMap_coefficient.x = float(normalMap); }
    void setNormalCoefficient(float normalCoefficient) { normalMap_coefficient.y = normalCoefficient; }
    void setDiffuseMap(TextureID const diffuseMap) { diffuseMap_coefficient.x = float(diffuseMap); }
    void setDiffuseCoefficient(float diffuseCoefficient) { diffuseMap_coefficient.y = diffuseCoefficient; }
    void setSpecularMap(TextureID const specularMap) { specularMap_coefficient_exponent.x = float(specularMap); }
    void setSpecularCoefficient(float specularCoefficient) { specularMap_coefficient_exponent.y = specularCoefficient; }
    void setShininessExponent(float shininessExponent) { specularMap_coefficient_exponent.z = shininessExponent; }
    void setReflectionMap(TextureID const reflectionMap) { reflectionMap_reflectance.x = float(reflectionMap); }
    void setReflectance(float reflectance) { reflectionMap_reflectance.y = reflectance; }

    virtual void* GetDataLayoutBeginPtr() override { return &alphaMap_opacity; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4) * 5; }

    Vec4 alphaMap_opacity;
    Vec4 normalMap_coefficient;
    Vec4 diffuseMap_coefficient;
    Vec4 reflectionMap_reflectance;
    Vec4 specularMap_coefficient_exponent;
};

#endif
