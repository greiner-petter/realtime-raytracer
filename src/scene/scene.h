#ifndef SCENE_H
#define SCENE_H

#include "common/color.h"
#include "common/Types.h"
#include "scene/Sphere.h"

class Scene {
public:
  Scene() = default;
  virtual ~Scene();

  void ConvertSceneToGPUData();
  byte* GetGPUData() const { return GPUData; }
  size_t GetGPUDataSize() const { return GPUDataSize; }
public:
  std::vector<Sphere> spheres;

private:
  byte* GPUData = nullptr;
  size_t GPUDataSize = 0;
};

#endif
