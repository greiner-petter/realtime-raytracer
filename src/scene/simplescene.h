#ifndef SIMPLESCENE_H
#define SIMPLESCENE_H

#include "scene/scene.h"

class SimpleScene : public Scene {

public:
  // Raytracing functions
  bool findIntersection(Ray &ray) const override;
  bool findOcclusion(Ray &ray) const override;
};

#endif
