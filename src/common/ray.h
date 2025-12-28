#ifndef RAY_H
#define RAY_H

#include "common/common.h"
#include "common/vector2d.h"
#include "common/vector3d.h"
#include <atomic>

// Forward declaration
class Primitive;

struct Ray {
  friend class Scene;

  // Components
  Vector3d origin;         // o
  Vector3d direction;      // d
  float length = INFINITY; // t
  Primitive const *primitive = nullptr;
  Vector3d normal;
  Vector2d surface;
  Vector3d tangent;
  Vector3d bitangent;

  // Constructor
  Ray(Vector3d const &origin = Vector3d(0, 0, 0), Vector3d const &direction = Vector3d(0, 0, 1)) : origin(origin), direction(normalized(direction)) {
    ++rayCount;
  }

  inline int getRemainingBounces() const { return remainingBounces; }

  static inline void resetRayCount() { rayCount = 0; }
  static inline int getRayCount() { return rayCount; }

private:
#ifndef ICG_RAY_BOUNCES
  int remainingBounces = 4;
#else
  int remainingBounces = ICG_RAY_BOUNCES;
#endif

  static std::atomic<int> rayCount;
};

#endif
