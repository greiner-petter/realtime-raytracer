#ifndef SCENE_H
#define SCENE_H

#include "common/color.h"
#include "common/ray.h"
#include "common/texture.h"
#include "common/vector3d.h"
#include <algorithm>
#include <memory>
#include <vector>

// Forward declarations
class Light;
class Shader;

class Scene {

public:
  // Constructor / Destructor
  Scene() = default;
  virtual ~Scene() = default;

  // Get
  std::vector<std::shared_ptr<Light>> const &lights() const { return this->lights_; }
  std::vector<std::shared_ptr<Primitive>> const &primitives() const { return this->primitives_; }

  // Set
  void setBackgroundColor(Color const &color) { this->backgroundColor = color; }
  void setEnvironmentMap(std::shared_ptr<Texture> const &map) { this->environmentMap = map; }

  // Setup functions
  void add(const std::shared_ptr<Light> &light);
  void add(const std::shared_ptr<Primitive> &primitive);

  void addObj(char const *fileName, Vector3d const &scale, Vector3d const &translation, const std::shared_ptr<Shader> &shader, bool flipU = false, bool flipV = false);

  // .obj loading function
  static std::vector<std::shared_ptr<Primitive>> loadObj(char const *fileName, Vector3d const &scale, Vector3d const &translation, const std::shared_ptr<Shader> &shader, bool flipU = false, bool flipV = true);

  // Raytracing functions
  Color traceRay(Ray &ray) const;
  virtual bool findIntersection(Ray &ray) const = 0;
  virtual bool findOcclusion(Ray &ray) const = 0;

protected:
  Color backgroundColor;
  std::shared_ptr<Texture> environmentMap;

private:
  std::vector<std::shared_ptr<Light>> lights_;
  std::vector<std::shared_ptr<Primitive>> primitives_;
};

#endif
