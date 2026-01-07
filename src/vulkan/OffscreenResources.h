#ifndef OFFSCREEN_RESOURCES_H
#define OFFSCREEN_RESOURCES_H

#include <vulkan/vulkan.h>

class OffscreenResources {
public:
    static void Init();
    static void Cleanup();
    static void Resize();

    static VkImageView GetImageView() { return imageView; }
    static VkImage GetImage() { return image; }

private:
    inline static VkImage image = VK_NULL_HANDLE;
    inline static VkDeviceMemory memory = VK_NULL_HANDLE;
    inline static VkImageView imageView = VK_NULL_HANDLE;
};

#endif
