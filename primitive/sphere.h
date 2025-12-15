#ifndef SPHERE_H
#define SPHERE_H

#include "primitive/primitive.h"

class Sphere : public Primitive {

public:
  // Constructor
  Sphere(std::shared_ptr<Shader> const &shader);
  Sphere(Vector3d const &center, float radius, std::shared_ptr<Shader> const &shader);

  // Set
  void setCenter(Vector3d const &center) { this->center = center; }
  void setRadius(float radius) { this->radius = radius; }

  // Primitive functions
  bool intersect(Ray &ray) const override;

  // Bounding box
  float minimumBounds(int dimension) const override;
  float maximumBounds(int dimension) const override;

protected:
  Vector3d center;
  float radius;
};

#endif
