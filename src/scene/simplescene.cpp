#include "scene/simplescene.h"
#include "primitive/primitive.h"
#include "shader/shader.h"

bool SimpleScene::findIntersection(Ray &ray) const {
  bool hit = false;
  for (auto i : this->primitives())
    hit |= i->intersect(ray);
  return hit;
}

bool SimpleScene::findOcclusion(Ray &ray) const {
  for (auto i : this->primitives())
    if (i->intersect(ray) && !i->shader()->isTransparent())
      return true;
  return false;
}
