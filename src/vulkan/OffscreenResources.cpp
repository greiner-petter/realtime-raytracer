#include "OffscreenResources.h"
#include "VulkanContext.h"
#include "Swapchain.h"
#include "common/Log.h"
#include "common/Params.h"

void OffscreenResources::Init() {
    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = OffscreenResources::GetWidth();
    imageInfo.extent.height = OffscreenResources::GetHeight();
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
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
    viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
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

void OffscreenResources::Clear() {
    VkCommandBuffer cmd = VulkanContext::BeginSingleTimeCommands();

    // Transition to TRANSFER_DST for clearing
    VkImageMemoryBarrier toTransferDst = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    toTransferDst.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransferDst.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    toTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toTransferDst.image = image;
    toTransferDst.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransferDst);

    // Clear to black
    VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdClearColorImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

    // Transition back to TRANSFER_SRC
    VkImageMemoryBarrier toTransferSrc = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    toTransferSrc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransferSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toTransferSrc.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toTransferSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    toTransferSrc.image = image;
    toTransferSrc.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransferSrc);

    VulkanContext::EndSingleTimeCommands(cmd);
}

uint32_t OffscreenResources::GetWidth() {
    return Params::GetWidth();
}

uint32_t OffscreenResources::GetHeight() {
    return Params::GetHeight();
}