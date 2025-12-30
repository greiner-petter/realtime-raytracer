#include "SSBO.h"
#include <vulkan/vulkan.h>
#include "common/Window.h"
#include "common/Log.h"
#include <vector>

extern VkBool32 GetMemoryType(uint32_t typeBits, VkFlags properties, uint32_t* typeIndex);
extern VkDevice device;

std::vector<std::shared_ptr<SSBO>> g_SSBOs;

SSBO::~SSBO() {
    if (m_Buffer != VK_NULL_HANDLE || m_DeviceMemory != VK_NULL_HANDLE)  {
        Destroy();
    }
}

std::shared_ptr<SSBO> SSBO::Create(uint32_t binding, VkDeviceSize size) {
    auto ssbo = std::shared_ptr<SSBO>(new SSBO());
    g_SSBOs.push_back(ssbo);
    ssbo->m_Binding = binding;
    Window::GetInstance()->SetResizedFlag(true);

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    vkCreateBuffer(device, &bufferInfo, nullptr, &ssbo->m_Buffer);
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, ssbo->m_Buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocInfo.memoryTypeIndex);

    vkAllocateMemory(device, &allocInfo, nullptr, &ssbo->m_DeviceMemory);
    vkBindBufferMemory(device, ssbo->m_Buffer, ssbo->m_DeviceMemory, 0);
    
    ssbo->UploadData(nullptr, 0); // Initialize with empty data

    return ssbo;
}

void SSBO::UploadData(void* InDataPointer, size_t InSize) {
    void* data;
    vkMapMemory(device, m_DeviceMemory, 0, InSize, 0, &data);
    memcpy(data, InDataPointer, InSize);
    vkUnmapMemory(device, m_DeviceMemory);
}

void SSBO::Destroy() {
    RT_ASSERT(device != VK_NULL_HANDLE, "Vulkan device is not valid");

    vkDestroyBuffer(device, m_Buffer, nullptr);
    vkFreeMemory(device, m_DeviceMemory, nullptr);
    m_Buffer = VK_NULL_HANDLE;
    m_DeviceMemory = VK_NULL_HANDLE;
}