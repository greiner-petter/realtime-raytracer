#ifndef MIRRORSHADER_H
#define MIRRORSHADER_H

#include "shader/shader.h"

class MirrorShader : public Shader {

public:
  // Constructor
  MirrorShader();

  // Shader functions
  Color shade(Scene const &scene, Ray const &ray) const override;
};

#endif
