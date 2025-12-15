#ifndef INFINITEPLANE_H
#define INFINITEPLANE_H

#include "primitive/primitive.h"

class InfinitePlane : public Primitive {

public:
  // Constructor
  InfinitePlane(std::shared_ptr<Shader> const &shader);
  InfinitePlane(Vector3d const &origin, Vector3d const &normal, std::shared_ptr<Shader> const &shader);

  // Set
  void setOrigin(Vector3d const &origin) { this->origin = origin; }
  void setNormal(Vector3d const &normal) { this->normal = normalized(normal); }

  // Primitive functions
  bool intersect(Ray &ray) const override;

  // Bounding box
  float minimumBounds(int dimension) const override;
  float maximumBounds(int dimension) const override;

protected:
  Vector3d origin, normal;
};

#endif
