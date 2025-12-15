#ifndef AMBIENTLIGHT_H
#define AMBIENTLIGHT_H

#include "light/light.h"

class AmbientLight : public Light {

public:
  // Constructor
  AmbientLight(float intensity, Color color = Color(1, 1, 1)) : Light(intensity, color) {}

  // Light functions
  Illumination illuminate(Scene const &scene, Ray const &ray) const override;
};

#endif
