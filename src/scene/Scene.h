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

struct Node {
  Node() : dimension(0), split(0), startIndex(0), primitiveCount(0) {}

  inline bool isLeaf() const {
    return child[0] == nullptr && child[1] == nullptr;
  }


  // Branch split
  std::unique_ptr<Node> child[2];
  int dimension;
  float split;

  // Leaf primitives
  uint32_t startIndex;
  uint32_t primitiveCount;
};

struct GPUKDNode {
  int left;
  int right;
  int axis;
  float split;
  int firstPrim;
  int primCount;
};

class Scene {
public:
  Scene() = default;
  virtual ~Scene();
  static void CreateGPUBuffers();
  void BuildTree(int maximumDepth = 10, int minimumNumberOfPrimitives = 2);

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

  std::unique_ptr<Node> Build(const Vec3& minimumBounds, const Vec3& maximumBounds, int start, int end /* [start, end) */, int depth);
  std::vector<GPUKDNode> FlattenKDTree() const;
  void UploadTreeToGPU();

private:
  inline static std::shared_ptr<UniformBuffer> uniformBuffer;
  inline static std::shared_ptr<SSBO> kdTreeSSBO;
  inline static std::shared_ptr<SSBO> primitiveSSBO;
  inline static std::shared_ptr<SSBO> sphereSSBO;
  inline static std::shared_ptr<SSBO> triangleSSBO;
  inline static std::shared_ptr<SSBO> planeSSBO;

  std::unique_ptr<Node> root;
  int maximumDepth;
  int minimumNumberOfPrimitives;
  Vec4 absoluteMinimum, absoluteMaximum; // xyz + padding

  bool m_IsBufferDirty = true;
};

#endif
