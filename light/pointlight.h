#ifndef POINTLIGHT_H
#define POINTLIGHT_H

#include "light/light.h"

class PointLight : public Light {

public:
  PointLight(Vector3d const &position, float intensity, Color const &color = Color(1, 1, 1));

  // Set
  void setPosition(Vector3d const &position) { this->position = position; }

  // Light functions
  Illumination illuminate(Scene const &scene, Ray const &ray) const override;

protected:
  Vector3d position;
};

#endif
