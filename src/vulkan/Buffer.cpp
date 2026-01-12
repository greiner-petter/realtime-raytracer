#include "Buffer.h"
#include "VulkanContext.h"
#include "common/Log.h"
#include <vulkan/vulkan.h>

void Buffer::UploadData(void* InDataPointer, size_t InSize) {
    if (InSize == 0 || InDataPointer == nullptr)
        return;
    
    void* data;
    vkMapMemory(VulkanContext::GetDevice(), m_DeviceMemory, 0, InSize, 0, &data);
    std::memcpy(data, InDataPointer, InSize);
    vkUnmapMemory(VulkanContext::GetDevice(), m_DeviceMemory);
}

void* Buffer::MapData(size_t InSize) {
    void* data;
    vkMapMemory(VulkanContext::GetDevice(), m_DeviceMemory, 0, InSize, 0, &data);
    return data;
}

void Buffer::UnmapData() {
    vkUnmapMemory(VulkanContext::GetDevice(), m_DeviceMemory);
}

void Buffer::Destroy() {
    if (VulkanContext::GetDevice() != VK_NULL_HANDLE) {
        if (m_Buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(VulkanContext::GetDevice(), m_Buffer, nullptr);
            m_Buffer = VK_NULL_HANDLE;
        }
        if (m_DeviceMemory != VK_NULL_HANDLE) {
            vkFreeMemory(VulkanContext::GetDevice(), m_DeviceMemory, nullptr);
            m_DeviceMemory = VK_NULL_HANDLE;
        }
    }
}

void Buffer::Create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(VulkanContext::GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        RT_ERROR("failed to create buffer!"); exit(1);
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(VulkanContext::GetDevice(), buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = VulkanContext::FindMemoryType(memReqs.memoryTypeBits, properties);

    if (vkAllocateMemory(VulkanContext::GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        RT_ERROR("failed to allocate buffer memory!"); exit(1);
    }

    vkBindBufferMemory(VulkanContext::GetDevice(), buffer, bufferMemory, 0);
}

void Buffer::DestroyAllBuffers() {
    for (auto& buffer : g_Buffers) {
        buffer->Destroy();
    }
    g_Buffers.clear();
}

std::shared_ptr<SSBO> SSBO::Create(uint32_t binding, VkDeviceSize size) {
    return Buffer::Create<SSBO>(binding, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

std::shared_ptr<UniformBuffer> UniformBuffer::Create(uint32_t binding, VkDeviceSize size) {
    return Buffer::Create<UniformBuffer>(binding, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}