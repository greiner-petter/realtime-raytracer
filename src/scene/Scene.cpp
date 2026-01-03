#include "scene/Scene.h"
#include "common/Log.h"
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

UBO uniformBufferData;

void Scene::CreateGPUBuffers() {
  uniformBuffer = UniformBuffer::Create(0, sizeof(uniformBufferData));
  primitiveSSBO = SSBO::Create(1);
  sphereSSBO = SSBO::Create(2);
  triangleSSBO = SSBO::Create(3);
}

Scene::~Scene() {
  
}

void Scene::ConvertSceneToGPUData() {
  // SPHERES BUFFER
  size_t GPUDataSize = sizeof(uint32_t)           // sphere count
              + sizeof(uint32_t) * 3              // padding to align to 16 bytes
              + sizeof(Vec4) * spheres.size();  // sphere data (16 bytes each)
  
  void* sphereDataGPU = sphereSSBO->MapData(GPUDataSize);
  
  // Copy sphere count
  const uint32_t sphereCount = static_cast<uint32_t>(spheres.size());
  std::memcpy(sphereDataGPU, &sphereCount, sizeof(uint32_t));

  // Copy sphere data
  for (size_t i = 0; i < spheres.size(); ++i) {
      const Sphere& sphere = spheres[i];
      std::memcpy(static_cast<byte*>(sphereDataGPU) + sizeof(uint32_t) * 4 + i * sizeof(Vec4), &sphere.center_radius, sizeof(Vec4));
  }
  sphereSSBO->UnmapData();


  // TRIANGLES BUFFER
  GPUDataSize = sizeof(uint32_t)           // triangle count
              + sizeof(uint32_t) * 3      // padding to align to 16 bytes
              + sizeof(Vec4) * 15 * triangles.size();  // triangle data (15 * 16 bytes each)

  void* triangleDataGPU = triangleSSBO->MapData(GPUDataSize);

  // Copy triangle count
  const uint32_t triangleCount = static_cast<uint32_t>(triangles.size());
  std::memcpy(triangleDataGPU, &triangleCount, sizeof(uint32_t));
  // Copy triangle data
  for (size_t i = 0; i < triangles.size(); ++i) {
      Triangle& triangle = triangles[i];
      byte* dstPtr = static_cast<byte*>(triangleDataGPU) + sizeof(uint32_t) * 4 + i * sizeof(Vec4) * 15;
      std::memcpy(dstPtr, triangle.GetTriangleData(), 15 * sizeof(Vec4));
  }
  triangleSSBO->UnmapData();


  // PRIMITIVES BUFFER
  struct GPUPrimitive {
      uint32_t type;
      int32_t  index;
      int32_t  materialID;
  };
  GPUDataSize = 4 + sizeof(GPUPrimitive) * spheres.size() + sizeof(GPUPrimitive) * triangles.size();
  void* primitiveDataGPU = primitiveSSBO->MapData(GPUDataSize);

  // Copy primitive count
  const uint32_t primitiveCount = static_cast<uint32_t>(spheres.size() + triangles.size());
  std::memcpy(primitiveDataGPU, &primitiveCount, sizeof(uint32_t));

  // Copy primitive data
  // primitives start immediately after count
  
  GPUPrimitive* primDst = reinterpret_cast<GPUPrimitive*>(reinterpret_cast<byte*>(primitiveDataGPU) + 4);

  for (size_t i = 0; i < spheres.size(); ++i) {
      primDst[i].type       = static_cast<uint32_t>(PrimitiveType::Sphere);
      primDst[i].index      = static_cast<int32_t>(i);
      primDst[i].materialID = spheres[i].materialID;
  }
  for (size_t i = 0; i < triangles.size(); ++i) {
      primDst[spheres.size() + i].type       = static_cast<uint32_t>(PrimitiveType::Triangle);
      primDst[spheres.size() + i].index      = static_cast<int32_t>(i);
      primDst[spheres.size() + i].materialID = triangles[i].materialID;
  }

  primitiveSSBO->UnmapData();
}

void Scene::UpdateGPUBuffers() {
    ConvertSceneToGPUData();
    uniformBufferData.u_resolution = glm::vec2(Window::GetInstance()->GetWidth(), Window::GetInstance()->GetHeight());
    uniformBufferData.u_aspectRatio = float(Window::GetInstance()->GetHeight()) / float(Window::GetInstance()->GetWidth());

    uniformBuffer->UploadData(&uniformBufferData, sizeof(uniformBufferData));

    SetBufferDirty(false);
}