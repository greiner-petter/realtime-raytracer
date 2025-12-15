#include "light/light.h"
#include "scene/scene.h"
#include "shader/cooktorranceshader.h"

CookTorranceShader::CookTorranceShader(Color const &diffCol, Color const &ctCol, float IOR, float roughness, float diffCoeff, float ctCoeff) : diffuseColor(diffCol * diffCoeff), ctColor(ctCol * ctCoeff), F0(IOR), m(roughness) {}

float CookTorranceShader::D(float NdotH) const {
  // Beckmann distribution
  float const r2 = m * m;
  float const NdotH2 = NdotH * NdotH;
  return expf((NdotH2 - 1.0f) / (r2 * NdotH2)) / (4.0f * r2 * powf(NdotH, 4.0f));
}

float CookTorranceShader::F(float VdotH) const {
  // Schlicks approximation
  return F0 + (1.0f - F0) * powf(1.0f - VdotH, 5);
}

float CookTorranceShader::G(float NdotH, float NdotV, float VdotH, float NdotL) const { return std::min(1.0f, std::min(2.0f * NdotH * NdotV / VdotH, 2.0f * NdotH * NdotL / VdotH)); }

Color CookTorranceShader::shade(Scene const &scene, Ray const &ray) const {
  Color fragmentColor;

  if (m >= 0.0f) {
    // Accumulate the light over all light sources
    for (const auto &light : scene.lights()) {
      Light::Illumination illum;
      illum = light->illuminate(scene, ray);

      float const NdotL = std::max(0.0f, dotProduct(-illum.direction, ray.normal));
      if (NdotL <= 0.0f)
        continue;

      // Diffuse term
      Color const diffuse = this->diffuseColor / float(PI);
      fragmentColor += diffuse * NdotL * illum.color;

      // Cook-Torrance term
      // half angle vector
      Vector3d const H = normalized(-illum.direction - ray.direction);
      float const NdotH = std::max(0.0f, dotProduct(ray.normal, H));
      float const NdotV = std::max(0.0f, dotProduct(ray.normal, -ray.direction));
      float const VdotH = std::max(0.0f, dotProduct(-ray.direction, H));

      if (NdotV * NdotL > EPSILON) {
        Color const specular = this->ctColor * (F(VdotH) * D(NdotH) * G(NdotH, NdotV, VdotH, NdotL)) / (float(PI) * NdotV * NdotL);

        fragmentColor += specular * NdotL * illum.color;
      }
    }
  }

  return fragmentColor;
}