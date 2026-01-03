#ifndef BUFFER_H
#define BUFFER_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "common/Types.h"
#include "common/Window.h"

#define SSBO_DEFAULT_SIZE (16 * 1024 * 1024) // 16 MB

extern uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
extern VkDevice device;

class Buffer {
public:
    virtual ~Buffer();
    void UploadData(void* InDataPointer, size_t InSize);
    void* MapData(size_t InSize);
    void UnmapData();
    virtual void Destroy();
    static void DestroyAllBuffers();
    static std::vector<std::shared_ptr<Buffer>> GetAllBuffers() { return g_Buffers; }

    uint32_t GetBindingPoint() const { return m_Binding; }
    VkDescriptorBufferInfo* GetBufferInfo() { return &m_BufferInfo; }
    VkBuffer GetBuffer() const { return m_Buffer; }
    virtual VkDescriptorType GetDescriptorType() const = 0;

protected:
    template <typename T>
    static std::shared_ptr<T> Create(uint32_t binding, VkDeviceSize size, VkBufferUsageFlags usageFlags) {
        auto buffer = std::make_shared<T>();
        g_Buffers.push_back(buffer);

        buffer->m_Binding = binding;
        Window::GetInstance()->SetResizedFlag(true);

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usageFlags;
        vkCreateBuffer(device, &bufferInfo, nullptr, &buffer->m_Buffer);

        VkDescriptorBufferInfo info{};
        info.buffer = buffer->m_Buffer;
        info.offset = 0;
        info.range  = VK_WHOLE_SIZE;
        buffer->m_BufferInfo = info;

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, buffer->m_Buffer, &memReqs);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkAllocateMemory(device, &allocInfo, nullptr, &buffer->m_DeviceMemory);
        vkBindBufferMemory(device, buffer->m_Buffer, buffer->m_DeviceMemory, 0);

        return buffer;
    }

    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_BufferInfo = {};
    uint32_t m_Binding = 0;

    inline static std::vector<std::shared_ptr<Buffer>> g_Buffers;
};

class SSBO : public Buffer {
public:
    static std::shared_ptr<SSBO> Create(uint32_t binding, VkDeviceSize size = SSBO_DEFAULT_SIZE);
    virtual VkDescriptorType GetDescriptorType() const override { return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; }
};

class UniformBuffer : public Buffer {
public:
    static std::shared_ptr<UniformBuffer> Create(uint32_t binding, VkDeviceSize size = sizeof(float) * 16);
    virtual VkDescriptorType GetDescriptorType() const override { return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; }
};

#endif