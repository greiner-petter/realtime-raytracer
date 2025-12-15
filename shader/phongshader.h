#ifndef PHONGSHADER_H
#define PHONGSHADER_H

#include "shader/shader.h"

class PhongShader : public Shader {

public:
  // Constructor
  PhongShader(Color const &diffuseColor, float diffuseCoefficient, Color const &specularColor,
              float specularCoefficient, float shininessExponent);

  // Shader functions
  Color shade(Scene const &scene, Ray const &ray) const override;

private:
  Color diffuseColor;
  float diffuseCoefficient;

  Color specularColor;
  float specularCoefficient;
  float shininessExponent;
};

#endif
