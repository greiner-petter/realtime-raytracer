#ifndef CAMERA_H
#define CAMERA_H

#include "common/ray.h"

class Camera {

public:
  // Constructor / Destructor
  Camera() = default;
  virtual ~Camera() = default;

  // Camera functions
  virtual Ray createRay(float x, float y) const = 0;
};

#endif
