#ifndef VULKAN_API_H
#define VULKAN_API_H

#include <memory>

class VulkanAPI {
public:
    static std::shared_ptr<class Scene> SetupVulkan();
    
    static void CreateInstance();
    static void CreateWindowSurface();
    static void CreateLogicalDevice();
    static void CreateCommandPool();
    static void CreateSwapChain();
    static void CreateImageViews();
    static void CreateVertexBuffer();
    static void CreateDescriptorPool();
    static void CreateDescriptorSet();
    static void CreateGraphicsPipeline();
    static void CreateCommandBuffers();

    static void OnWindowSizeChanged();
    static void Draw();

    static void ClearSwapChain();
    static void ClearCommandBuffers();
    static void ClearPipeline();
    static void FullCleanUp();

    static void Refresh();
};

#endif