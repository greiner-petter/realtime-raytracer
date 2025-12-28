#ifndef LAMBERTSHADER_H
#define LAMBERTSHADER_H

#include "shader/shader.h"

class LambertShader : public Shader {

public:
  // Constructor
  LambertShader(Color const &diffuseColor = Color(1, 1, 1));

  // Shader functions
  Color shade(Scene const &scene, Ray const &ray) const override;

protected:
  Color diffuseColor;
};

#endif
