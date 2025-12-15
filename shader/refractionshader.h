#ifndef REFRACTIONSHADER_H
#define REFRACTIONSHADER_H

#include "shader/shader.h"

class RefractionShader : public Shader {

public:
  // Constructor
  RefractionShader(float indexInside, float indexOutside);

  // Shader functions
  Color shade(Scene const &scene, Ray const &ray) const override;
  bool isTransparent() const override;

private:
  float indexInside;
  float indexOutside;
};

#endif
