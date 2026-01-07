#include "OffscreenResources.h"
#include "VulkanContext.h"
#include "Swapchain.h"
#include "common/Log.h"

void OffscreenResources::Init() {
    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = Swapchain::GetExtent().width;
    imageInfo.extent.height = Swapchain::GetExtent().height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(VulkanContext::GetDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
        RT_ERROR("failed to create offscreen image"); exit(1);
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(VulkanContext::GetDevice(), image, &memReqs);

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = VulkanContext::FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(VulkanContext::GetDevice(), &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        RT_ERROR("failed to allocate offscreen memory"); exit(1);
    }
    vkBindImageMemory(VulkanContext::GetDevice(), image, memory, 0);

    VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    if (vkCreateImageView(VulkanContext::GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        RT_ERROR("failed to create offscreen view"); exit(1);
    }

    VkCommandBuffer cmd = VulkanContext::BeginSingleTimeCommands();
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.image = image;
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    VulkanContext::EndSingleTimeCommands(cmd);
}

void OffscreenResources::Cleanup() {
    vkDestroyImageView(VulkanContext::GetDevice(), imageView, nullptr);
    vkDestroyImage(VulkanContext::GetDevice(), image, nullptr);
    vkFreeMemory(VulkanContext::GetDevice(), memory, nullptr);
}

void OffscreenResources::Resize() {
    Cleanup();
    Init();
}
