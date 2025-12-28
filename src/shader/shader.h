#ifndef SHADER_H
#define SHADER_H

#include "common/color.h"
#include "common/ray.h"

// Forward declarations
class Scene;

class Shader {
public:
  // Constructor / Desctructor
  Shader() = default;
  virtual ~Shader() = default;

  // Get
  virtual bool isTransparent() const { return false; }

  // Shader functions
  virtual Color shade(Scene const &scene, Ray const &ray) const = 0;
};

#endif
