#include "light/light.h"
#include "scene/scene.h"
#include "shader/brdfshader.h"

BrdfShader::BrdfShader(char const *fileName, Color const &scale)
    : scale(scale), brdf(std::make_unique<BRDFRead>(fileName)) {}

Color BrdfShader::shade(Scene const &scene, Ray const &ray) const {
  // Calculate theta and phi
  float thetaIn = std::acos(dotProduct(-ray.normal, ray.direction));
  float phiIn = 0.0f;

  // Derive local coordinate system
  Vector3d const x = crossProduct(-ray.direction, ray.normal);
  Vector3d const y = crossProduct(ray.normal, x);

  // Accumulate the light over all light sources
  Color illuminationColor;
  for (const auto &light : scene.lights()) {
    Light::Illumination illum;
    illum = light->illuminate(scene, ray);

    // Diffuse term
    float const cosine = dotProduct(-illum.direction, ray.normal);
    if (cosine > 0) {
      Color color;

      // Avoid numeric instability
      if (cosine < 1) {
        float const thetaOut = std::acos(cosine);

        // Project outgoing vector into local coordinate system
        Vector3d const c = crossProduct(-illum.direction, ray.normal);
        float const phiOut = std::atan2(dotProduct(c, y), dotProduct(c, x));

        color = Color(brdf->lookupBrdfValues(thetaIn, phiIn, thetaOut, phiOut));
      } else {
        color = Color(brdf->lookupBrdfValues(thetaIn, phiIn, 0, 0));
      }

      // Calculate colors
      Color const diffuseColor = scale * color * cosine;
      illuminationColor += diffuseColor * illum.color;
    }
  }
  return illuminationColor;
}
