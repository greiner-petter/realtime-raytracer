#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>

class Swapchain {
public:
    static void Init();
    static void Cleanup();
    static void Recreate();

    static VkSwapchainKHR GetHandle() { return swapChain; }
    static VkExtent2D GetExtent() { return swapChainExtent; }
    static VkFormat GetFormat() { return SWAPCHAIN_FORMAT; }
    static const std::vector<VkImage>& GetImages() { return swapChainImages; }

private:
    static void Create();

    inline static VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    inline static std::vector<VkImage> swapChainImages;
    inline static VkExtent2D swapChainExtent;
    static const VkFormat SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
};

#endif
