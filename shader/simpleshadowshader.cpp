#include "light/light.h"
#include "scene/scene.h"
#include "shader/simpleshadowshader.h"

SimpleShadowShader::SimpleShadowShader(Color const &objectColor) : objectColor(objectColor) {}

Color SimpleShadowShader::shade(Scene const &scene, Ray const &ray) const {
  Color fragmentColor;

  // Accumulate the light over all light sources
  for (const auto &light : scene.lights())
    fragmentColor += light->illuminate(scene, ray).color;

  return fragmentColor * this->objectColor;
}
