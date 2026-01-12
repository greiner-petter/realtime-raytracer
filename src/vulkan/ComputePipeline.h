#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include <vulkan/vulkan.h>

class ComputePipeline {
public:
    static void Init();
    static void Cleanup();
    static void UpdateDescriptorSets();

    static VkPipeline GetPipeline() { return pipeline; }
    static VkPipelineLayout GetLayout() { return pipelineLayout; }
    static VkDescriptorSet GetDescriptorSet() { return descriptorSet; }

private:
    static void CreateDescriptorPool();
    static void CreateDescriptorSetLayout();
    static void CreatePipeline();

    inline static VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    inline static VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    inline static VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    inline static VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    inline static VkPipeline pipeline = VK_NULL_HANDLE;
};

#endif
