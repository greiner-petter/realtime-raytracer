#ifndef SIMPLESHADOWSHADER_H
#define SIMPLESHADOWSHADER_H

#include "shader/shader.h"

class SimpleShadowShader : public Shader {

public:
  // Constructor
  SimpleShadowShader(Color const &objectColor);

  // Shader functions
  Color shade(Scene const &scene, Ray const &ray) const override;

private:
  Color objectColor;
};

#endif
