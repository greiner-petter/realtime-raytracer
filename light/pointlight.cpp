#include "light/pointlight.h"
#include "scene/scene.h"

PointLight::PointLight(Vector3d const &position, float intensity, Color const &color) : Light(intensity, color), position(position) {}

Light::Illumination PointLight::illuminate(Scene const &scene, Ray const &ray) const {
  Vector3d const target = ray.origin + (ray.length - LGT_EPS) * ray.direction;

  // Illumination object
  Illumination illum;
  illum.direction = normalized(target - this->position);

  // Precompute the distance from the light source
  float const distance = length(target - this->position);

  // Define a secondary ray from the surface point to the light source.
  Ray lightRay;
  lightRay.origin = target;
  lightRay.direction = -illum.direction;
  lightRay.length = distance - LGT_EPS;

  // If the target is not in shadow...
  if (!scene.findOcclusion(lightRay))
    // ... compute the attenuation and light color
    illum.color = 1.0f / (distance * distance) * this->color * this->intensity;
  return illum;
}
