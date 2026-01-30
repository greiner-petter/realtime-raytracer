#include "ComputePipeline.h"
#include "VulkanContext.h"
#include "OffscreenResources.h"
#include "vulkan/Buffer.h"
#include "vulkan/ShaderCompiler.h"
#include "common/Log.h"

void ComputePipeline::Init() {
    CreateDescriptorPool();
    CreateDescriptorSetLayout();
    UpdateDescriptorSets();
    CreatePipeline();
}

void ComputePipeline::Cleanup() {
    vkDestroyPipeline(VulkanContext::GetDevice(), pipeline, nullptr);
    vkDestroyPipelineLayout(VulkanContext::GetDevice(), pipelineLayout, nullptr);
    vkDestroyDescriptorPool(VulkanContext::GetDevice(), descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(VulkanContext::GetDevice(), descriptorSetLayout, nullptr);
}

void ComputePipeline::RecreatePipeline() {
    VulkanContext::DeviceWaitIdle();
    vkDestroyPipeline(VulkanContext::GetDevice(), pipeline, nullptr);
    vkDestroyPipelineLayout(VulkanContext::GetDevice(), pipelineLayout, nullptr);
    CreatePipeline();
}

void ComputePipeline::CreateDescriptorPool() {
    VkDescriptorPoolSize sizes[] = { 
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 }, 
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 }, 
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 } 
    };
    VkDescriptorPoolCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    info.maxSets = 10;
    info.poolSizeCount = 3;
    info.pPoolSizes = sizes;
    vkCreateDescriptorPool(VulkanContext::GetDevice(), &info, nullptr, &descriptorPool);
}

void ComputePipeline::CreateDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    
    VkDescriptorSetLayoutBinding imageBinding{};
    imageBinding.binding = 255;
    imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageBinding.descriptorCount = 1;
    imageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings.push_back(imageBinding);

    auto buffers = Buffer::GetAllBuffers();
    for (const auto& buff : buffers) {
        VkDescriptorSetLayoutBinding b = {};
        b.binding = buff->GetBindingPoint();
        b.descriptorType = buff->GetDescriptorType();
        b.descriptorCount = 1;
        b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings.push_back(b);
    }

    VkDescriptorSetLayoutCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.bindingCount = (uint32_t)bindings.size();
    info.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(VulkanContext::GetDevice(), &info, nullptr, &descriptorSetLayout);
}

void ComputePipeline::UpdateDescriptorSets() {
    if (descriptorSet == VK_NULL_HANDLE) {
        VkDescriptorSetAllocateInfo alloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        alloc.descriptorPool = descriptorPool;
        alloc.descriptorSetCount = 1;
        alloc.pSetLayouts = &descriptorSetLayout;
        vkAllocateDescriptorSets(VulkanContext::GetDevice(), &alloc, &descriptorSet);
    }

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = OffscreenResources::GetImageView();

    VkWriteDescriptorSet imageWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    imageWrite.dstSet = descriptorSet;
    imageWrite.dstBinding = 255;
    imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageWrite.descriptorCount = 1;
    imageWrite.pImageInfo = &imageInfo;

    std::vector<VkWriteDescriptorSet> writes = { imageWrite };
    auto buffers = Buffer::GetAllBuffers();
    for (const auto& buff : buffers) {
        VkWriteDescriptorSet bw = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        bw.dstSet = descriptorSet;
        bw.dstBinding = buff->GetBindingPoint();
        bw.descriptorType = buff->GetDescriptorType();
        bw.descriptorCount = 1;
        bw.pBufferInfo = buff->GetBufferInfo();
        writes.push_back(bw);
    }
    vkUpdateDescriptorSets(VulkanContext::GetDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

VkShaderModule CreateShaderModule(const ShaderBinary& bin) {
    VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    info.codeSize = bin.GetSizeInBytes();
    info.pCode = (uint32_t*)bin.GetData();
    VkShaderModule mod;
    vkCreateShaderModule(VulkanContext::GetDevice(), &info, nullptr, &mod);
    return mod;
}

void ComputePipeline::CreatePipeline() {
    ShaderBinary compBin("ShaderCache/Raytracer.comp.glsl.spv");
    VkShaderModule compShader = CreateShaderModule(compBin);

    VkPipelineShaderStageCreateInfo stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = compShader;
    stage.pName = "main";

    VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout;
    
    vkCreatePipelineLayout(VulkanContext::GetDevice(), &layoutInfo, nullptr, &pipelineLayout);

    VkComputePipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipelineInfo.stage = stage;
    pipelineInfo.layout = pipelineLayout;

    vkCreateComputePipelines(VulkanContext::GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
    vkDestroyShaderModule(VulkanContext::GetDevice(), compShader, nullptr);
}
