#ifndef MATERIALSHADER_H
#define MATERIALSHADER_H

#include "common/texture.h"
#include "shader/shader.h"
#include <memory>

class MaterialShader : public Shader {

public:
  // Constructor
  MaterialShader();

  // Set
  void setAlphaMap(std::shared_ptr<Texture> const &alphaMap) { this->alphaMap = alphaMap; }
  void setOpacity(float opacity) { this->opacity = opacity; }
  void setNormalMap(std::shared_ptr<Texture> const &normalMap) { this->normalMap = normalMap; }
  void setNormalCoefficient(float normalCoefficient) { this->normalCoefficient = normalCoefficient; }
  void setDiffuseMap(std::shared_ptr<Texture> const &diffuseMap) { this->diffuseMap = diffuseMap; }
  void setDiffuseCoefficient(float diffuseCoefficient) { this->diffuseCoefficient = diffuseCoefficient; }
  void setSpecularMap(std::shared_ptr<Texture> const &specularMap) { this->specularMap = specularMap; }
  void setSpecularCoefficient(float specularCoefficient) { this->specularCoefficient = specularCoefficient; }
  void setShininessExponent(float shininessExponent) { this->shininessExponent = shininessExponent; }
  void setReflectionMap(std::shared_ptr<Texture> const &reflectionMap) { this->reflectionMap = reflectionMap; }
  void setReflectance(float reflectance) { this->reflectance = reflectance; }

  // Shader functions
  Color shade(Scene const &scene, Ray const &ray) const override;
  bool isTransparent() const override;

private:
  std::shared_ptr<Texture> alphaMap;
  float opacity;

  std::shared_ptr<Texture> normalMap;
  float normalCoefficient;

  std::shared_ptr<Texture> diffuseMap;
  float diffuseCoefficient;

  std::shared_ptr<Texture> reflectionMap;
  float reflectance;

  std::shared_ptr<Texture> specularMap;
  float specularCoefficient;
  float shininessExponent;
};

#endif
