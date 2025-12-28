#include "common/ray.h"
#include "primitive/sphere.h"

// Constructor /////////////////////////////////////////////////////////////////

Sphere::Sphere(std::shared_ptr<Shader> const &shader) : Primitive(shader), radius(0.5f) {}

Sphere::Sphere(Vector3d const &center, float radius, std::shared_ptr<Shader> const &shader)
    : Primitive(shader), center(center), radius(radius) {}

// Primitive functions /////////////////////////////////////////////////////////

bool Sphere::intersect(Ray &ray) const {
  // Use the definitions from the lecture
  Vector3d const difference = ray.origin - this->center;
  float const a = 1.0f;
  float const b = 2.0f * dotProduct(ray.direction, difference);
  float const c = dotProduct(difference, difference) - this->radius * this->radius;
  float const discriminant = b * b - 4 * a * c;

  // Test whether the ray could intersect at all
  if (discriminant < 0)
    return false;
  float const root = std::sqrt(discriminant);

  // Stable solution
  float const q = -0.5f * (b < 0 ? (b - root) : (b + root));
  float const t0 = q / a;
  float const t1 = c / q;
  float t = std::min(t0, t1);
  if (t < EPSILON)
    t = std::max(t0, t1);

  // Test whether this is the foremost primitive in front of the camera
  if (t < EPSILON || ray.length < t)
    return false;

  // Calculate the normal
  Vector3d const hitPoint = ray.origin + t * ray.direction;
  ray.normal = normalized(hitPoint - this->center);

  // Calculate the surface position and tangent vector
  float const phi = std::acos(ray.normal.y);
  float const rho = std::atan2(ray.normal.z, ray.normal.x) + PI;
  ray.surface = Vector2d(rho / (2 * PI), phi / PI);
  ray.tangent = Vector3d(std::sin(rho), 0, std::cos(rho));
  ray.bitangent = normalized(crossProduct(ray.normal, ray.tangent));

  // Set the new length and the current primitive
  ray.length = t;
  ray.primitive = this;

  // True, because the primitive was hit
  return true;
}

// Bounding box ////////////////////////////////////////////////////////////////

float Sphere::minimumBounds(int dimension) const { return this->center[dimension] - this->radius; }

float Sphere::maximumBounds(int dimension) const { return this->center[dimension] + this->radius; }
