#include "scene/Scene.h"
#include "common/Log.h"
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

UBO uniformBufferData;

Scene::Scene() {
  uniformBuffer = UniformBuffer::Create(0, sizeof(uniformBufferData));
  primitiveSSBO = SSBO::Create(1);
  sphereSSBO = SSBO::Create(2);
}

Scene::~Scene() {
  
}

void Scene::ConvertSceneToGPUData() {
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

  struct GPUPrimitive {
      uint32_t type;
      int32_t  index;
      int32_t  materialID;
  };
  GPUDataSize = 4 + sizeof(GPUPrimitive) * spheres.size();
  void* primitiveDataGPU = primitiveSSBO->MapData(GPUDataSize);

  // Copy primitive count
  const uint32_t primitiveCount = static_cast<uint32_t>(spheres.size());
  std::memcpy(primitiveDataGPU, &primitiveCount, sizeof(uint32_t));

  // Copy primitive data
  // primitives start immediately after count
  
  GPUPrimitive* primDst = reinterpret_cast<GPUPrimitive*>(reinterpret_cast<byte*>(primitiveDataGPU) + 4);

  for (size_t i = 0; i < spheres.size(); ++i) {
      primDst[i].type       = static_cast<uint32_t>(PrimitiveType::Sphere);
      primDst[i].index      = static_cast<int32_t>(i);
      primDst[i].materialID = spheres[i].materialID;
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