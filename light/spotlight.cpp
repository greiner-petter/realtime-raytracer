#include "light/spotlight.h"
#include "scene/scene.h"

SpotLight::SpotLight(Vector3d const &position, Vector3d const &direction, float alphaMin, float alphaMax, float intensity, Color const &color)
    : Light(intensity, color), position(position), direction(normalized(direction)), alphaMin(alphaMin), alphaMax(alphaMax) {}

Light::Illumination SpotLight::illuminate(Scene const &scene, Ray const &ray) const {
  Vector3d const target = ray.origin + (ray.length - LGT_EPS) * ray.direction;

  // Illumination object
  Illumination illum;
  illum.direction = normalized(target - this->position);

  // Precompute the distance from the light source
  float const distance = length(target - this->position);

  // Define a secondary ray from the surface point to the light source
  Ray lightRay;
  lightRay.origin = target;
  lightRay.direction = -illum.direction;
  lightRay.length = distance - LGT_EPS;

  // Determine the angle of the inner cone
  float const alpha = std::fabs(std::acos(dotProduct(illum.direction, this->direction)) * 180.0f / float(PI));

  // If the target is within the cone...
  if (this->alphaMax > alpha) {
    // ... and not in shadow ...
    if (!scene.findOcclusion(lightRay)) {
      // ... compute the attenuation and light color ...
      illum.color = 1.0f / (distance * distance) * this->color * this->intensity;
      // ... then compute the falloff towards the edge of the cone
      if (this->alphaMin < alpha)
        illum.color *= 1.0f - (alpha - this->alphaMin) / (this->alphaMax - this->alphaMin);
    }
  }
  return illum;
}
