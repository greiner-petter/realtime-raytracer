#include "light/ambientlight.h"

Light::Illumination AmbientLight::illuminate(Scene const &scene, Ray const &ray) const {
  return {this->color * this->intensity, -ray.normal};
}
