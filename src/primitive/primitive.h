#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "common/ray.h"
#include "shader/shader.h"
#include <algorithm>
#include <memory>

class Primitive {

public:
  // Constructor / Destructor
  Primitive(const std::shared_ptr<Shader> &shader) : shader_(shader) {}
  virtual ~Primitive() = default;

  // Get
  std::shared_ptr<Shader> shader() const { return this->shader_; }

  // Primitive functions
  virtual bool intersect(Ray &ray) const = 0;

  // Bounding box
  virtual float minimumBounds(int dimension) const = 0;
  virtual float maximumBounds(int dimension) const = 0;

private:
  std::shared_ptr<Shader> shader_;
};

#endif
