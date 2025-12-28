#ifndef COOKTORRANCESHADER_H
#define COOKTORRANCESHADER_H

#include "shader/shader.h"

class CookTorranceShader : public Shader {
public:
  CookTorranceShader(Color const &diffuseColor, Color const &ctColor, float IOR, float roughness,
                     float diffuseCoefficient = PI / 2.0f, float ctCoefficient = PI / 2.0f);
  // Shader functions
  Color shade(Scene const &scene, Ray const &ray) const override;

private:
  float D(float NdotH) const;
  float F(float VdotH) const;
  float G(float NdotH, float NdotV, float VdotH, float NdotL) const;

  Color diffuseColor;

  Color ctColor;

  float F0;
  float m;
};

#endif