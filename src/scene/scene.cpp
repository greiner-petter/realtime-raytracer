#include "scene/Scene.h"
#include "common/Log.h"
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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