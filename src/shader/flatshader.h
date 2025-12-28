#ifndef FLATSHADER_H
#define FLATSHADER_H

#include "shader/shader.h"

class FlatShader : public Shader {

public:
  // Constructor
  FlatShader(Color const &objectColor);

  // Shader functions
  virtual Color shade(Scene const &scene, Ray const &ray) const;

protected:
  Color objectColor;
};

#endif
