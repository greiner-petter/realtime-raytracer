#include "scene/Scene.h"
#include "common/Log.h"
#include "common/Window.h"
#include "common/Params.h"
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int FlattenNode(const Node* node, std::vector<GPUKDNode>& outNodes) {
    int myIndex = static_cast<int>(outNodes.size());

    // Reserve space (we'll fill children later)
    outNodes.push_back({});

    GPUKDNode& gpuNode = outNodes.back();

    if (node->isLeaf()) {
        // Leaf node
        gpuNode.left      = -1;
        gpuNode.right     = -1;
        gpuNode.axis      = -1;
        gpuNode.split     = 0.0f;
        gpuNode.firstPrim = static_cast<int>(node->startIndex);
        gpuNode.primCount = static_cast<int>(node->primitiveCount);
    } else {
        // Inner node
        gpuNode.axis      = node->dimension;
        gpuNode.split     = node->split;
        gpuNode.firstPrim = -1;
        gpuNode.primCount = 0;

        // Flatten children
        gpuNode.left  = FlattenNode(node->child[0].get(), outNodes);
        gpuNode.right = FlattenNode(node->child[1].get(), outNodes);
    }

    return myIndex;
}

std::vector<GPUKDNode> Scene::FlattenKDTree() const {
    std::vector<GPUKDNode> nodes;
    nodes.reserve(m_Primitives.size() * 2); // heuristic
    FlattenNode(root.get(), nodes);
    return nodes;
}

UBO uniformBufferData;

void Scene::CreateGPUBuffers() {
  uniformBuffer = UniformBuffer::Create(0, sizeof(uniformBufferData));
  kdTreeSSBO = SSBO::Create(1);

  primitiveSSBO = SSBO::Create(10);
  sphereSSBO = SSBO::Create(11);
  triangleSSBO = SSBO::Create(12);
  planeSSBO = SSBO::Create(13);
  boxSSBO = SSBO::Create(14);

  shaderSSBO = SSBO::Create(20);
  flatSSBO = SSBO::Create(21);
  refractionSSBO = SSBO::Create(22);
  mirrorSSBO = SSBO::Create(23);
  simpleShadowSSBO = SSBO::Create(24);

  lightSSBO = SSBO::Create(30);
  pointSSBO = SSBO::Create(31);
}

void Scene::ConvertSceneToGPUData() {
  WriteBufferForType(m_Primitives, PrimitiveType::Sphere, *sphereSSBO);
  WriteBufferForType(m_Primitives, PrimitiveType::Triangle, *triangleSSBO);
  WriteBufferForType(m_Primitives, PrimitiveType::InfinitePlane, *planeSSBO);
  WriteBufferForType(m_Primitives, PrimitiveType::Box, *boxSSBO);

  WriteBufferForType(m_Shaders, ShaderType::FlatShader, *flatSSBO);
  WriteBufferForType(m_Shaders, ShaderType::MirrorShader, *mirrorSSBO);
  WriteBufferForType(m_Shaders, ShaderType::RefractionShader, *refractionSSBO);
  WriteBufferForType(m_Shaders, ShaderType::SimpleShadowShader, *simpleShadowSSBO);

  WriteBufferForType(m_Lights, LightType::PointLight, *pointSSBO);

  // PRIMITIVES BUFFER
  struct GPUPrimitive {
      uint32_t primitiveType;
      int32_t  primitiveIndex;
      uint32_t shaderType;
      int32_t  shaderIndex;
  };
  size_t GPUDataSize = 4 + sizeof(GPUPrimitive) * m_Primitives.size();
  void* primitiveDataGPU = primitiveSSBO->MapData(GPUDataSize);

  // Copy primitive count
  const uint32_t primitiveCount = static_cast<uint32_t>(m_Primitives.size());
  std::memcpy(primitiveDataGPU, &primitiveCount, sizeof(uint32_t));

  // Copy primitive data
  // primitives start immediately after count
  
  GPUPrimitive* primDst = reinterpret_cast<GPUPrimitive*>(reinterpret_cast<byte*>(primitiveDataGPU) + 4);

  for (size_t i = 0; i < m_Primitives.size(); ++i) {
      RT_ASSERT(m_Primitives[i]->type != PrimitiveType::None, "Primitive type is None");
      primDst[i].primitiveType = static_cast<uint32_t>(m_Primitives[i]->type);
      primDst[i].primitiveIndex = m_Primitives[i]->index;
      primDst[i].shaderType = static_cast<uint32_t>(m_Primitives[i]->shader->type);
      primDst[i].shaderIndex = m_Primitives[i]->shader->index;
  }

  primitiveSSBO->UnmapData();

    struct GPULight {
        uint32_t lightType;
        int32_t  lightIndex;
    };
    size_t GPULightDataSize = 4 + sizeof(GPULight) * m_Lights.size();
    void* lightDataGPU = lightSSBO->MapData(GPULightDataSize);

    const uint32_t lightCount = static_cast<uint32_t>(m_Lights.size());
    std::memcpy(lightDataGPU, &lightCount, sizeof(uint32_t));

    GPULight* lightDst = reinterpret_cast<GPULight*>(reinterpret_cast<byte*>(lightDataGPU) + 4);

    for (size_t i = 0; i < m_Lights.size(); ++i) {
        RT_ASSERT(m_Lights[i]->type != LightType::None, "Light type is None");
        lightDst[i].lightType = static_cast<uint32_t>(m_Lights[i]->type);
        lightDst[i].lightIndex = m_Lights[i]->index;
    }

    lightSSBO->UnmapData();
}

void Scene::UploadTreeToGPU() {
  std::vector<GPUKDNode> flatNodes = FlattenKDTree();
  size_t GPUDataSize = sizeof(Vec4) * 2 + sizeof(GPUKDNode) * flatNodes.size();

  void* DataGPU = kdTreeSSBO->MapData(GPUDataSize);

  // Copy AABB
  std::memcpy(DataGPU, &absoluteMinimum, sizeof(Vec4));
  std::memcpy(static_cast<byte*>(DataGPU) + sizeof(Vec4), &absoluteMaximum, sizeof(Vec4));

  // Copy data
  std::memcpy(static_cast<byte*>(DataGPU) + sizeof(Vec4) * 2,
              flatNodes.data(),
              sizeof(GPUKDNode) * flatNodes.size());

  kdTreeSSBO->UnmapData();
}

void Scene::UpdateGPUBuffers() {
  // Always update uniform buffer
  uniformBufferData.u_resolution = Params::IsInteractiveMode() 
    ? glm::vec2(Window::GetInstance()->GetWidth(), Window::GetInstance()->GetHeight()) 
    : glm::vec2(Params::GetWidth(), Params::GetHeight());

  uniformBufferData.u_aspectRatio = uniformBufferData.u_resolution.y / uniformBufferData.u_resolution.x;
  uniformBufferData.u_Seed = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  uniformBuffer->UploadData(&uniformBufferData, sizeof(uniformBufferData));
  uniformBufferData.u_SampleIndex++;
  // update scene buffers only if dirty

  if (IsBufferDirty()) {
    BuildTree(maximumDepth, minimumNumberOfPrimitives);
    UploadTreeToGPU();
    ConvertSceneToGPUData();
    SetBufferDirty(false);
  }
}

void Scene::BuildTree(int maximumDepth, int minimumNumberOfPrimitives) {
  // Set the new depth and number of primitives
  this->maximumDepth = maximumDepth;
  this->minimumNumberOfPrimitives = minimumNumberOfPrimitives;

  // Determine the bounding box of the kD-Tree
  this->absoluteMinimum = Vec4(+INFINITY, +INFINITY, +INFINITY, 0.0f);
  this->absoluteMaximum = Vec4(-INFINITY, -INFINITY, -INFINITY, 0.0f);
  for (const auto &primitive : m_Primitives) {
    for (int d = 0; d < 3; ++d) {
      this->absoluteMinimum[d] = std::min(this->absoluteMinimum[d], primitive->minimumBounds(d));
      this->absoluteMaximum[d] = std::max(this->absoluteMaximum[d], primitive->maximumBounds(d));
    }
  }

  // Recursively build the kD-Tree
  root = this->Build(this->absoluteMinimum, this->absoluteMaximum, 0, m_Primitives.size(), 0);
  RT_INFO("{0} primitives organized into tree", m_Primitives.size());
}

std::unique_ptr<Node> Scene::Build(const Vec3& minimumBounds, const Vec3& maximumBounds, int start, int end /* [start, end) */, int depth) {
    const int count = end - start;
    Vec3 extent = maximumBounds - minimumBounds;

    // Choose split axis (widest dimension)
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;

    // Leaf conditions
    if (depth >= maximumDepth ||
        count <= minimumNumberOfPrimitives ||
        extent[axis] <= EPSILON) {

        auto leaf = std::make_unique<Node>();
        leaf->startIndex = start;
        leaf->primitiveCount = count;
        return leaf;
    }

    // Compute split position (median of centroids)
    std::vector<float> centroids;
    centroids.reserve(count);
    for (int i = start; i < end; ++i) {
        float c = 0.5f * (
            m_Primitives[i]->minimumBounds(axis) +
            m_Primitives[i]->maximumBounds(axis)
        );
        centroids.push_back(c);
    }

    std::nth_element(
        centroids.begin(),
        centroids.begin() + centroids.size() / 2,
        centroids.end()
    );

    float split = centroids[centroids.size() / 2];

    // In-place partition
    int mid = start;
    
    for (int i = start; i < end; ++i) {
        float center = 0.5f * (
            m_Primitives[i]->minimumBounds(axis) +
            m_Primitives[i]->maximumBounds(axis)
        );

        if (center < split) {
            std::swap(m_Primitives[i], m_Primitives[mid]);
            mid++;
        }
    }

    // Abort split if it failed (all on one side)
    if (mid == start || mid == end) {
        auto leaf = std::make_unique<Node>();
        leaf->startIndex = start;
        leaf->primitiveCount = count;
        return leaf;
    }

    // Create inner node
    auto node = std::make_unique<Node>();
    node->dimension = axis;
    node->split = split;

    // Compute child bounds
    Vec3 leftMax = maximumBounds;
    Vec3 rightMin = minimumBounds;
    leftMax[axis] = split;
    rightMin[axis] = split;

    node->child[0] = Build(minimumBounds, leftMax, start, mid, depth + 1);
    node->child[1] = Build(rightMin, maximumBounds, mid, end, depth + 1);

    return node;
}

void Scene::ClearScene() {
  uniformBufferData.u_SampleIndex = 0;
  m_Primitives.clear();
  m_Shaders.clear();
  //m_Lights.clear();
  root.reset(nullptr);
  m_IsBufferDirty = true;
}