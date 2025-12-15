#include "primitive/infiniteplane.h"
#include <cmath>

// Constructor /////////////////////////////////////////////////////////////////

InfinitePlane::InfinitePlane(std::shared_ptr<Shader> const &shader) : Primitive(shader), normal(0, 1, 0) {}

InfinitePlane::InfinitePlane(Vector3d const &origin, Vector3d const &normal, std::shared_ptr<Shader> const &shader)
    : Primitive(shader), origin(origin), normal(normal) {}

// Primitive functions /////////////////////////////////////////////////////////

bool InfinitePlane::intersect(Ray &ray) const {
  float const cosine = dotProduct(ray.direction, this->normal);

  // Make sure the ray is not coming from the other side (backface culling).
  // Note: We only use backface culling for InfinitePlanes, because we have
  // some special features planned that rely on backfaces for other primitives.
  if (cosine > 0)
    return false;

  // Determine the distance at which the ray intersects the plane
  float const t = dotProduct(this->origin - ray.origin, this->normal) / cosine;

  // Test whether this is the foremost primitive in front of the camera
  if (t < EPSILON || ray.length < t)
    return false;

  // Set the normal
  ray.normal = this->normal;

  // Set the new length and the current primitive
  ray.length = t;
  ray.primitive = this;

  // True, because the primitive was hit
  return true;
}

// Bounding box ////////////////////////////////////////////////////////////////

float InfinitePlane::minimumBounds(int dimension) const {
  if (this->normal[dimension] == 1.0f) // plane is orthogonal to the axis
    return this->origin[dimension] - EPSILON;
  else
    return -INFINITY;
}

float InfinitePlane::maximumBounds(int dimension) const {
  if (this->normal[dimension] == 1.0f) // plane is orthogonal to the axis
    return this->origin[dimension] + EPSILON;
  else
    return +INFINITY;
}
