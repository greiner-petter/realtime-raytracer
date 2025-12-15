#include "common/ray.h"
#include "primitive/box.h"
#include <cmath>
#include <utility>

// Constructor /////////////////////////////////////////////////////////////////

Box::Box(std::shared_ptr<Shader> const &shader) : Primitive(shader), size(Vector3d(1, 1, 1)) {}

Box::Box(Vector3d const &center, Vector3d const &size, std::shared_ptr<Shader> const &shader)
    : Primitive(shader), center(center), size(size) {}

// Primitive functions /////////////////////////////////////////////////////////

bool Box::intersect(Ray &ray) const {
  // Project the ray onto the box
  Vector3d const minBounds = this->center - this->size / 2;
  Vector3d const maxBounds = this->center + this->size / 2;
  Vector3d t1 = componentQuotient(minBounds - ray.origin, ray.direction);
  Vector3d t2 = componentQuotient(maxBounds - ray.origin, ray.direction);

  // Determine the intersection points (tNear, tFar)
  // We also have to remember the intersection axes (tNearIndex, tFarIndex)
  float tNear = -INFINITY;
  float tFar = +INFINITY;
  int tNearIndex = 0;
  int tFarIndex = 0;
  for (int d = 0; d < 3; ++d) {

    // Test the trivial case (and to avoid division by zero errors)
    if (ray.direction[d] == 0 && (ray.origin[d] < minBounds[d] || ray.origin[d] > maxBounds[d]))
      return false;

    // Swap the bounds if necessary
    if (t1[d] > t2[d])
      std::swap(t1[d], t2[d]);

    // Check for the near intersection
    if (t1[d] > tNear) {
      tNear = t1[d];
      tNearIndex = d;
    }

    // Check for the far intersection
    if (t2[d] < tFar) {
      tFar = t2[d];
      tFarIndex = d;
    }

    // Check whether we missed the box completely
    if (tFar < 0 || tNear > tFar)
      return false;
  }

  // Check whether we are on the outside or on the inside of the box
  float const t = (tNear >= 0 ? tNear : tFar);
  int const tIndex = tNear >= 0 ? tNearIndex : tFarIndex;

  // Test whether this is the foremost primitive in front of the camera
  if (ray.length < t)
    return false;

  // Calculate the normal
  ray.normal = Vector3d(0, 0, 0);
  // Flip the normal if we are on the inside
  // Note: This is necessary to ensure we don't backface-cull an environment cube; for compatibility with
  //       the refraction shader, the normal should *always* point outwards.
  ray.normal[tIndex] = std::copysignf(1.0f, ray.direction[tIndex]) * (tNear < 0.0f ? +1.0f : -1.0f);

  // Calculate the surface position and tangent vector
  Vector3d const target = ray.origin + t * ray.direction;
  Vector3d const surface = componentQuotient(target - minBounds, maxBounds - minBounds);
  if (tIndex == 0) {
    ray.surface = Vector2d(surface[2], surface[1]);
    ray.tangent = Vector3d(0, 0, 1);
  } else if (tIndex == 1) {
    ray.surface = Vector2d(surface[0], surface[2]);
    ray.tangent = Vector3d(1, 0, 0);
  } else {
    ray.surface = Vector2d(surface[0], surface[1]);
    ray.tangent = Vector3d(1, 0, 0);
  }

  // Set the new length and the current primitive
  ray.length = t;
  ray.primitive = this;

  // True, because the primitive was hit
  return true;
}

// Bounding box ////////////////////////////////////////////////////////////////

float Box::minimumBounds(int dimension) const { return this->center[dimension] - this->size[dimension] / 2; }

float Box::maximumBounds(int dimension) const { return this->center[dimension] + this->size[dimension] / 2; }
