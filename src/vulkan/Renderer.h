#ifndef RENDERER_H
#define RENDERER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class Renderer {
public:
    static void Init();
    static void Cleanup();
    static void Draw();
    static void OnWindowSizeChanged();
    static void SaveCurrentFrameToDisk(const std::string& filePath);

private:
    static void CreateSyncObjects();
    static void CreateCommandBuffers();
    static void RecordCommandBuffers();
    static void FreeCommandBuffers();

    inline static std::vector<VkCommandBuffer> commandBuffers;
    inline static VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    inline static VkSemaphore renderingFinishedSemaphore = VK_NULL_HANDLE;
    inline static VkFence inFlightFence = VK_NULL_HANDLE;
};

#endif
