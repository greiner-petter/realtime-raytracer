#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "primitive/primitive.h"

class Triangle : public Primitive {

public:
  // Constructor
  Triangle(std::shared_ptr<Shader> const &shader);
  Triangle(Vector3d const &a, Vector3d const &b, Vector3d const &c, std::shared_ptr<Shader> const &shader);
  Triangle(Vector3d const &a, Vector3d const &b, Vector3d const &c, Vector3d const &na, Vector3d const &nb, Vector3d const &nc, std::shared_ptr<Shader> const &shader);
  Triangle(Vector3d const &a, Vector3d const &b, Vector3d const &c, Vector3d const &na, Vector3d const &nb, Vector3d const &nc, Vector2d const &ta, Vector2d const &tb, Vector2d const &tc, std::shared_ptr<Shader> const &shader);
  Triangle(Vector3d const &a, Vector3d const &b, Vector3d const &c, Vector3d const &na, Vector3d const &nb, Vector3d const &nc, Vector3d const &tana, Vector3d const &tanb, Vector3d const &tanc, Vector3d const &ba, Vector3d const &bb,
           Vector3d const &bc, Vector2d const &ta, Vector2d const &tb, Vector2d const &tc, std::shared_ptr<Shader> const &shader);

  // Set
  void setVertex(int index, Vector3d const &vertex) { this->vertex[index] = vertex; }
  void setNormal(int index, Vector3d const &normal) { this->normal[index] = normalized(normal); }
  void setTangent(int index, Vector3d const &tangent) { this->tangent[index] = normalized(tangent); }
  void setBitangent(int index, Vector3d const &bitangent) { this->bitangent[index] = normalized(bitangent); }
  void setSurface(int index, Vector2d const &surface) { this->surface[index] = surface; }

  // Get
  Vector3d getPosition(size_t index) { return this->vertex[index]; }
  Vector3d getNormal(size_t index) { return this->normal[index]; }
  Vector3d getTangent(size_t index) { return this->tangent[index]; }
  Vector3d getBitangent(size_t index) { return this->bitangent[index]; }
  Vector2d getTexCoord(size_t index) { return this->surface[index]; }

  // Primitive functions
  bool intersect(Ray &ray) const override;
  bool intersectArea(Ray &ray) const;

  // Bounding box
  float minimumBounds(int dimension) const override;
  float maximumBounds(int dimension) const override;

protected:
  Vector3d vertex[3];
  Vector3d normal[3];
  Vector3d tangent[3];
  Vector3d bitangent[3];
  Vector2d surface[3];
};

#endif
