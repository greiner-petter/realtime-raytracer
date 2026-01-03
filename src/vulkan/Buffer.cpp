#include "Buffer.h"
#include <vulkan/vulkan.h>
#include "common/Window.h"
#include "common/Log.h"
#include <vector>

Buffer::~Buffer() {
    if (m_Buffer != VK_NULL_HANDLE || m_DeviceMemory != VK_NULL_HANDLE)  {
        Destroy();
    }
}

void Buffer::UploadData(void* InDataPointer, size_t InSize) {
    if (InSize == 0 || InDataPointer == nullptr)
        return;
    
    void* data;
    vkMapMemory(device, m_DeviceMemory, 0, InSize, 0, &data);
    memcpy(data, InDataPointer, InSize);
    vkUnmapMemory(device, m_DeviceMemory);
}

void* Buffer::MapData(size_t InSize) {
    void* data;
    vkMapMemory(device, m_DeviceMemory, 0, InSize, 0, &data);
    return data;
}

void Buffer::UnmapData() {
    vkUnmapMemory(device, m_DeviceMemory);
}

void Buffer::Destroy() {
    RT_ASSERT(device != VK_NULL_HANDLE, "Vulkan device is not valid");

    vkDestroyBuffer(device, m_Buffer, nullptr);
    vkFreeMemory(device, m_DeviceMemory, nullptr);
    m_Buffer = VK_NULL_HANDLE;
    m_DeviceMemory = VK_NULL_HANDLE;
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