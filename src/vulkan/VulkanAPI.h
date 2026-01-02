#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>

class VulkanAPI {
public:
    static std::shared_ptr<class Scene> SetupVulkan();
    
    static void CreateInstance();
    static void CreateWindowSurface();
    static void CreateLogicalDevice();
    static void CreateCommandPool();
    static void CreateVertexBuffer();
    static void CreateDescriptorPool();
    static void CreateDescriptorSet();
    

    static void CleanUp(bool fullClean);

    static void OnWindowSizeChanged();
    static void Draw();
};