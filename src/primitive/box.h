#ifndef BOX_H
#define BOX_H

#include "primitive/primitive.h"

class Box : public Primitive {

public:
  // Constructor
  Box(std::shared_ptr<Shader> const &shader);
  Box(Vector3d const &center, Vector3d const &size, std::shared_ptr<Shader> const &shader);

  // Set
  void setCenter(Vector3d const &center) { this->center = center; }
  void setSize(Vector3d const &size) { this->size = size; }

  // Primitive functions
  bool intersect(Ray &ray) const override;

  // Bounding box
  float minimumBounds(int dimension) const override;
  float maximumBounds(int dimension) const override;

protected:
  Vector3d center;
  Vector3d size;
};

#endif
