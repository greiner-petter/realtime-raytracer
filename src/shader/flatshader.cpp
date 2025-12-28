#include "shader/flatshader.h"

FlatShader::FlatShader(Color const &objectColor) : objectColor(objectColor) {}

Color FlatShader::shade(Scene const &scene, Ray const &ray) const {
  (void)ray; // ray is unused in this case, but we do not want a warning.
  return this->objectColor;
}
