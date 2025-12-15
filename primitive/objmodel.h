#ifndef OBJMODEL_H
#define OBJMODEL_H

#include "primitive/box.h"
#include <vector>

class ObjModel : public Primitive {
public:
  // Constructor
  ObjModel(std::shared_ptr<Shader> const &shader);
  ~ObjModel() override{};

  // Load object data
  void loadObj(char const *fileName, Vector3d const &scale = Vector3d(1, 1, 1),
               Vector3d const &translation = Vector3d(0, 0, 0));

  // Primitive functions
  bool intersect(Ray &ray) const override;

  // Bounding box
  float minimumBounds(int dimension) const override;
  float maximumBounds(int dimension) const override;

protected:
  Box boundingBox;
  std::vector<std::shared_ptr<Primitive>> primitives;
};

#endif
