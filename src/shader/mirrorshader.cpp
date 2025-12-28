#include "scene/scene.h"
#include "shader/mirrorshader.h"

MirrorShader::MirrorShader() {}

Color MirrorShader::shade(Scene const &scene, Ray const &ray) const {
  // Calculate the reflection vector
  Vector3d const reflection = ray.direction - 2 * dotProduct(ray.normal, ray.direction) * ray.normal;

  // Create a new reflection ray
  Ray reflectionRay = ray;
  reflectionRay.origin = ray.origin + (ray.length - REFR_EPS) * ray.direction;
  reflectionRay.direction = normalized(reflection);
  reflectionRay.length = INFINITY;
  reflectionRay.primitive = nullptr;

  // Send the new ray out into the scene and return the result
  return scene.traceRay(reflectionRay);
}
