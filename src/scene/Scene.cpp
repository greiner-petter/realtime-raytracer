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

void Scene::WriteBufferForPrimitiveType(PrimitiveType type, SSBO& ssbo) {
  size_t typeCount = 0;
  size_t sizeOfType = 0;
  uint32_t indiceIt = 0;
  std::vector<std::shared_ptr<Primitive>> primsitivesOfType;
  for (const auto& prim : m_Primitives) {
      if (prim->type == type) {
          prim->index = indiceIt++;
          primsitivesOfType.push_back(prim);
          typeCount++;
          sizeOfType = prim->GetDataSize();
      }
  }

  if (typeCount == 0) {
      return;
  }

  size_t GPUDataSize = sizeof(uint32_t)           // count
              + sizeof(uint32_t) * 3              // padding to align to 16 bytes
              + sizeOfType * typeCount;           // data
  
  void* DataGPU = ssbo.MapData(GPUDataSize);
  
  // Copy count
  const uint32_t count = static_cast<uint32_t>(typeCount);
  std::memcpy(DataGPU, &count, sizeof(uint32_t));

  // Copy data
  for (auto& prim : primsitivesOfType) {
      std::memcpy(static_cast<byte*>(DataGPU) + sizeof(uint32_t) * 4 + prim->index * sizeOfType, 
                  prim->GetDataLayoutBeginPtr(), 
                  sizeOfType);
  }
  ssbo.UnmapData();
}

void Scene::ConvertSceneToGPUData() {
  WriteBufferForPrimitiveType(PrimitiveType::Sphere, *sphereSSBO);
  WriteBufferForPrimitiveType(PrimitiveType::Triangle, *triangleSSBO);

  // PRIMITIVES BUFFER
  struct GPUPrimitive {
      uint32_t type;
      int32_t  index;
      int32_t  materialID;
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
      primDst[i].type       = static_cast<uint32_t>(m_Primitives[i]->type);
      primDst[i].index      = static_cast<int32_t>(m_Primitives[i]->index);
      primDst[i].materialID = m_Primitives[i]->materialID;
  }

  primitiveSSBO->UnmapData();
}

void Scene::UpdateGPUBuffers() {
  // Always update uniform buffer
  uniformBufferData.u_resolution = glm::vec2(Window::GetInstance()->GetWidth(), Window::GetInstance()->GetHeight());
  uniformBufferData.u_aspectRatio = float(Window::GetInstance()->GetHeight()) / float(Window::GetInstance()->GetWidth());
  uniformBuffer->UploadData(&uniformBufferData, sizeof(uniformBufferData));
  
  // update scene buffers only if dirty
  if (IsBufferDirty()) {
    ConvertSceneToGPUData();
    SetBufferDirty(false);
  }
}