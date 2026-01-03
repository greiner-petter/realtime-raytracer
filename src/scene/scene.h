#ifndef SCENE_H
#define SCENE_H

#include "common/color.h"
#include "common/Types.h"
#include "scene/Sphere.h"
#include "scene/Triangle.h"

#include "vulkan/Buffer.h"

struct UBO {
    glm::vec2 u_resolution;
    float u_aspectRatio;
    float u_FocusDistance = 1.0f;
    glm::vec4 u_CameraPosition = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 u_CameraForward = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    glm::vec4 u_CameraRight = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 u_CameraUp = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
};

class Scene {
public:
  Scene();
  virtual ~Scene();

  void ConvertSceneToGPUData();
  void UpdateGPUBuffers();
  bool IsBufferDirty() const { return m_IsBufferDirty; }
  void SetBufferDirty(bool dirty) { m_IsBufferDirty = dirty; }
public:
  std::vector<Sphere> spheres;
  std::vector<Triangle> triangles;

private:
  std::shared_ptr<UniformBuffer> uniformBuffer;
  std::shared_ptr<SSBO> primitiveSSBO;
  std::shared_ptr<SSBO> sphereSSBO;
  std::shared_ptr<SSBO> triangleSSBO;

  bool m_IsBufferDirty = true;
};

#endif
