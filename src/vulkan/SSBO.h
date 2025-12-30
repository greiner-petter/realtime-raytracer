#ifndef SSBO_H
#define SSBO_H

#include <vulkan/vulkan.h>
#include <memory>

#define SSBO_DEFAULT_SIZE (16 * 1024 * 1024) /* 16 MB */

class SSBO {
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
    uint32_t m_Binding;
    VkDescriptorBufferInfo m_BufferInfo;
public:
    ~SSBO();
    static std::shared_ptr<SSBO> Create(uint32_t binding, VkDeviceSize size = SSBO_DEFAULT_SIZE);
    void UploadData(void* InDataPointer, size_t InSize);

    VkDescriptorBufferInfo* GetBufferInfo() {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_Buffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = VK_WHOLE_SIZE;
        m_BufferInfo = bufferInfo;
        return &m_BufferInfo;
    }

    VkBuffer GetBuffer() const { return m_Buffer; }

public:
    void Destroy();
};

#endif