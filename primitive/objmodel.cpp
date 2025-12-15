#include "primitive/objmodel.h"
#include "primitive/box.h"
#include "primitive/triangle.h"
#include "scene/scene.h"

ObjModel::ObjModel(std::shared_ptr<Shader> const &shader) : Primitive(shader), boundingBox(Vector3d(-INFINITY, -INFINITY, -INFINITY), Vector3d(INFINITY, INFINITY, INFINITY), shader) {}

void ObjModel::loadObj(char const *fileName, Vector3d const &scale, Vector3d const &translation) {
  // Load faces
  this->primitives = Scene::loadObj(fileName, scale, translation, shader());

  // Extent of box
  Vector3d minVert(INFINITY, INFINITY, INFINITY);
  Vector3d maxVert(-INFINITY, -INFINITY, -INFINITY);

  // For each face, update the extent
  for (const auto &primitive : this->primitives) {
    minVert = Vector3d(std::min(minVert.x, primitive->minimumBounds(0)), std::min(minVert.y, primitive->minimumBounds(1)), std::min(minVert.z, primitive->minimumBounds(2)));
    maxVert = Vector3d(std::max(maxVert.x, primitive->maximumBounds(0)), std::max(maxVert.y, primitive->maximumBounds(1)), std::max(maxVert.z, primitive->maximumBounds(2)));
  }

  // Update the bounding box
  boundingBox.setCenter(0.5f * (maxVert + minVert));
  boundingBox.setSize(maxVert - minVert + Vector3d(SPLT_EPS, SPLT_EPS, SPLT_EPS));
}

bool ObjModel::intersect(Ray &ray) const {
  // Ray box intersection
  Ray boxRay = ray;
  if (boundingBox.intersect(boxRay)) {
    // ray primitive intersection
    bool hit = false;
    for (const auto &p : this->primitives) {
      hit |= p->intersect(ray);
    }
    return hit;
  }
  return false;
}

float ObjModel::minimumBounds(int dimension) const { return this->boundingBox.minimumBounds(dimension); }

float ObjModel::maximumBounds(int dimension) const { return this->boundingBox.maximumBounds(dimension); }
