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
  sceneSSBO = SSBO::Create(1);
}

Scene::~Scene() {
  if (GPUData) {
    delete[] GPUData;
    GPUData = nullptr;
  }
}

void Scene::ConvertSceneToGPUData() {
  RT_ASSERT(sizeof(Sphere) == 16 && sizeof(Sphere) == sizeof(Vec4), "Sphere size on CPU and GPU must match");

  GPUDataSize = sizeof(uint32_t)                  // sphere count
              + sizeof(uint32_t) * 3              // padding to align to 16 bytes
              + sizeof(Sphere) * spheres.size();  // sphere data (16 bytes each)
  
  if (GPUData) {
    delete[] GPUData;
  }
  GPUData = new byte[GPUDataSize];
  
  // Copy sphere count
  const uint32_t sphereCount = static_cast<uint32_t>(spheres.size());
  std::memcpy(GPUData, &sphereCount, sizeof(uint32_t));

  // Copy sphere data
  std::memcpy(GPUData + sizeof(uint32_t) * 4, spheres.data(), sizeof(Sphere) * spheres.size());
}

void Scene::UpdateGPUBuffers() {
    ConvertSceneToGPUData();
    uniformBufferData.u_resolution = glm::vec2(Window::GetInstance()->GetWidth(), Window::GetInstance()->GetHeight());
    uniformBufferData.u_aspectRatio = float(Window::GetInstance()->GetHeight()) / float(Window::GetInstance()->GetWidth());

    uniformBuffer->UploadData(&uniformBufferData, sizeof(uniformBufferData));
    sceneSSBO->UploadData(GetGPUData(), GetGPUDataSize());

    SetBufferDirty(false);
}