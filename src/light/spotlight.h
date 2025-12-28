#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H

#include "light/light.h"

class SpotLight : public Light {

public:
  // Constructor
  SpotLight(Vector3d const &position, Vector3d const &direction, float alphaMin, float alphaMax, float intensity,
            Color const &color = Color(1, 1, 1));

  // Set
  void setDirection(Vector3d const &direction) { this->direction = normalized(direction); }
  void setPosition(Vector3d const &position) { this->position = position; }
  void setAlphaMax(float alphaMax) { this->alphaMax = alphaMax; }
  void setAlphaMin(float alphaMin) { this->alphaMin = alphaMin; }

  // Light functions
  Illumination illuminate(Scene const &scene, Ray const &ray) const override;

protected:
  Vector3d position, direction;
  float alphaMin, alphaMax;
};

#endif
