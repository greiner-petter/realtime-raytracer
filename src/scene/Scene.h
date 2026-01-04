#ifndef SCENE_H
#define SCENE_H

#include "common/Types.h"
#include "scene/Primitive.h"

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
  Scene() = default;
  virtual ~Scene();
  static void CreateGPUBuffers();

  void WriteBufferForPrimitiveType(PrimitiveType type, SSBO& ssbo);
  void ConvertSceneToGPUData();
  void UpdateGPUBuffers();
  bool IsBufferDirty() const { return m_IsBufferDirty; }
  void SetBufferDirty(bool dirty) { m_IsBufferDirty = dirty; }
public:
  std::vector<std::shared_ptr<Primitive>> m_Primitives;

  template<typename T>
  void AddPrimitive(const T& primitive) {
      m_Primitives.push_back(std::make_shared<T>(primitive));
      m_IsBufferDirty = true;
  }

private:
  inline static std::shared_ptr<UniformBuffer> uniformBuffer;
  inline static std::shared_ptr<SSBO> primitiveSSBO;
  inline static std::shared_ptr<SSBO> sphereSSBO;
  inline static std::shared_ptr<SSBO> triangleSSBO;

  bool m_IsBufferDirty = true;
};

#endif
