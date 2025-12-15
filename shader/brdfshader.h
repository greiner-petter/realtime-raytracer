#ifndef BRDFSHADER_H
#define BRDFSHADER_H

#include "common/brdfread.h"
#include "shader/shader.h"
#include <memory>

class BrdfShader : public Shader {

public:
  // Constructor
  BrdfShader(char const *fileName, Color const &scale);
  ~BrdfShader() override = default;

  // Shader functions
  Color shade(Scene const &scene, Ray const &ray) const override;

private:
  Color scale;
  std::unique_ptr<BRDFRead> brdf;
};

#endif
