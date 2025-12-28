#ifndef LIGHT_H
#define LIGHT_H

#include "common/color.h"
#include "common/ray.h"

// Forward declarations
class Scene;

class Light {
public:
  // Illumination object
  struct Illumination {
    Color color;
    Vector3d direction;
  };

  // Constructor / Destructor
  Light(float intensity, Color const &color = Color(1, 1, 1)) : color(color), intensity(intensity) {}
  virtual ~Light() = default;

  // Set
  void setColor(Color const &color) { this->color = color; }
  void setIntensity(float intensity) { this->intensity = intensity; }

  // Light functions
  virtual Illumination illuminate(Scene const &scene, Ray const &ray) const = 0;

protected:
  Color color;
  float intensity;
};

#endif
