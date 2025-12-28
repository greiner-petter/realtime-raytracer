#include "light/light.h"
#include "scene/scene.h"
#include "shader/lambertshader.h"

LambertShader::LambertShader(Color const &diffuseColor) : diffuseColor(diffuseColor) {}

Color LambertShader::shade(Scene const &scene, Ray const &ray) const {
  Color fragmentColor;

  // Accumulate the light over all light sources
  for (const auto &light : scene.lights()) {
    Light::Illumination const illum = light->illuminate(scene, ray);
    // Diffuse term
    Color const diffuse = this->diffuseColor * std::max(dotProduct(-illum.direction, ray.normal), 0.0f);
    fragmentColor += diffuse * illum.color;
  }

  return fragmentColor;
}
