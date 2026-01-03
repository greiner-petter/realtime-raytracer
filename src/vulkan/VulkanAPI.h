#ifndef VULKAN_API_H
#define VULKAN_API_H

#include <memory>

class VulkanAPI {
public:
    static void SetupVulkan();
    
    static void CreateInstance();
    static void CreateWindowSurface();
    static void CreateLogicalDevice();
    static void CreateCommandPool();
    static void CreateSwapChain();
    static void CreateDescriptorPool();
    static void CreateDescriptorSet();
    static void CreateOffscreenResources();
    static void CreateComputePipeline();
    static void CreateCommandBuffers();

    static void OnWindowSizeChanged();
    static void Draw();

    static void ClearSwapChain();
    static void ClearCommandBuffers();
    static void ClearPipeline();
    static void ClearOffscreenResources();
    static void FullCleanUp();

    static void Refresh();
};

#endif