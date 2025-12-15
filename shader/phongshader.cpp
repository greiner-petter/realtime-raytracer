#include "light/light.h"
#include "scene/scene.h"
#include "shader/phongshader.h"

PhongShader::PhongShader(Color const &diffuseColor, float diffuseCoefficient, Color const &specularColor,
                         float specularCoefficient, float shininessExponent)
    : diffuseColor(diffuseColor), diffuseCoefficient(diffuseCoefficient), specularColor(specularColor),
      specularCoefficient(specularCoefficient), shininessExponent(shininessExponent) {}

Color PhongShader::shade(Scene const &scene, Ray const &ray) const {
  Color fragmentColor;

  // Calculate the reflection vector
  Vector3d const reflection = ray.direction - 2 * dotProduct(ray.normal, ray.direction) * ray.normal;

  // Accumulate the light over all light sources
  for (const auto &light : scene.lights()) {
    Light::Illumination illum;
    illum = light->illuminate(scene, ray);

    // Diffuse term
    Color const diffuse =
        this->diffuseCoefficient * this->diffuseColor * std::max(dotProduct(-illum.direction, ray.normal), 0.0f);
    fragmentColor += diffuse * illum.color;

    // Specular term
    float const cosine = dotProduct(-illum.direction, reflection);
    if (cosine > 0) {
      Color const specular = this->specularCoefficient * this->specularColor // highlight
                             * powf(cosine, this->shininessExponent);        // shininess factor
      fragmentColor += specular * illum.color;
    }
  }

  return fragmentColor;
}
