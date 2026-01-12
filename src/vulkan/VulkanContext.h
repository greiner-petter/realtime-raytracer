#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#include <vulkan/vulkan.h>

class VulkanContext {
public:
    static void Init();
    static void Cleanup();
    
    static VkDevice GetDevice() { return device; }
    static VkPhysicalDevice GetPhysicalDevice() { return physicalDevice; }
    static VkQueue GetGraphicsQueue() { return graphicsQueue; }
    static VkCommandPool GetCommandPool() { return commandPool; }
    static VkSurfaceKHR GetSurface() { return surface; }
    static uint32_t GetQueueFamilyIndex() { return graphicsQueueFamily; }

    static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    static VkCommandBuffer BeginSingleTimeCommands();
    static void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
    static void DeviceWaitIdle();

private:
    static void CreateInstance();
    static void CreateWindowSurface();
    static void CreateLogicalDevice();
    static void CreateCommandPool();

    inline static VkInstance instance = VK_NULL_HANDLE;
    inline static VkSurfaceKHR surface = VK_NULL_HANDLE;
    inline static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    inline static VkDevice device = VK_NULL_HANDLE;
    inline static VkQueue graphicsQueue = VK_NULL_HANDLE;
    inline static VkCommandPool commandPool = VK_NULL_HANDLE;
    inline static uint32_t graphicsQueueFamily = 0;
};

#endif
