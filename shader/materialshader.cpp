#include "light/light.h"
#include "scene/scene.h"
#include "shader/materialshader.h"
#include <cmath>

Vector3d tangentToWorldSpace(const Vector3d &surfaceNormal, const Vector3d &surfaceTangent, const Vector3d &surfaceBitangent, const Vector3d &textureNormal) {
  return textureNormal.x * surfaceTangent + textureNormal.y * surfaceBitangent + textureNormal.z * surfaceNormal;
}

MaterialShader::MaterialShader() : opacity(1.0f), normalCoefficient(1.0f), diffuseCoefficient(0.5f), reflectance(0.0f), specularCoefficient(0.5f), shininessExponent(8) {}

Color MaterialShader::shade(Scene const &scene, Ray const &ray) const {
  Color fragmentColor;

  // (Normal Map) Calculate the new normal vector
  Vector3d normal = ray.normal;
  if (this->normalMap) {
    Color const normalColor = this->normalMap->color(ray.surface);
    Vector3d const textureNormal = Vector3d(2.0f * normalColor.r, 2.0f * normalColor.g, 2.0f * normalColor.b) - Vector3d(1, 1, 1);
    normal = normalized(tangentToWorldSpace(normal, ray.tangent, ray.bitangent, normalized(textureNormal)) * this->normalCoefficient + (1.0f - this->normalCoefficient) * normal);
  }

  // Calculate the reflection vector
  Vector3d const reflection = normalized(ray.direction - 2 * dotProduct(normal, ray.direction) * normal);

  // (Diffuse-/Specular Map) Accumulate the light over all light sources
  for (const auto &light : scene.lights()) {

    // Retrieve an illumination object
    Light::Illumination illum = light->illuminate(scene, ray);

    // Diffuse term
    Color const diffuse = this->diffuseCoefficient * illum.color * std::max(dotProduct(-illum.direction, normal), 0.0f);
    if (this->diffuseMap)
      fragmentColor += diffuse * this->diffuseMap->color(ray.surface);
    else
      fragmentColor += diffuse;

    // Specular term
    float const cosine = dotProduct(-illum.direction, reflection);
    if (cosine > 0) {
      Color const specular = this->specularCoefficient * illum.color * std::pow(cosine, shininessExponent);
      if (this->specularMap)
        fragmentColor += specular * this->specularMap->color(ray.surface);
      else
        fragmentColor += specular;
    }
  }

  // (Reflection Map) Calculate the reflectance
  float reflectance = this->reflectance;
  if (this->reflectionMap)
    reflectance *= this->reflectionMap->color(ray.surface).r;
  if (reflectance > 0.0f) {
    // Create a new reflection ray
    Ray reflectionRay = ray;
    reflectionRay.origin = ray.origin + (ray.length - EPSILON) * ray.direction;
    reflectionRay.direction = reflection;
    reflectionRay.length = INFINITY;
    reflectionRay.primitive = nullptr;
    // Mix the object and the reflected image
    Color const reflectionColor = scene.traceRay(reflectionRay);
    fragmentColor = (1 - reflectance) * fragmentColor + reflectance * reflectionColor;
  }

  // (Alpha Map) Calculate the opacity
  float alpha = this->opacity;
  if (this->alphaMap)
    alpha *= this->alphaMap->color(ray.surface).r;
  if (alpha < 1) {
    // Create a new alpha ray
    Ray alphaRay = ray;
    alphaRay.origin = ray.origin + (ray.length + EPSILON) * ray.direction;
    alphaRay.length = INFINITY;
    alphaRay.primitive = nullptr;
    // Mix the foreground and background colors
    Color const backgroundColor = scene.traceRay(alphaRay);
    fragmentColor = alpha * fragmentColor + (1 - alpha) * backgroundColor;
  }

  return fragmentColor;
}

bool MaterialShader::isTransparent() const { return this->opacity < 1.0f || this->alphaMap; }
