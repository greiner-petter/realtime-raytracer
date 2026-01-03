#include "VulkanAPI.h"
#include "common/Log.h"
#include "common/Window.h"
#include "common/Types.h"
#include "vulkan/Buffer.h"
#include "vulkan/ShaderCompiler.h"
#include "scene/Scene.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

VkInstance instance = VK_NULL_HANDLE;
VkSurfaceKHR windowSurface = VK_NULL_HANDLE;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;
VkQueue graphicsQueue = VK_NULL_HANDLE; 
VkCommandPool commandPool = VK_NULL_HANDLE;
uint32_t graphicsQueueFamily = 0;
uint32_t presentQueueFamily = 0;

VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
VkVertexInputBindingDescription vertexBindingDescription = {};
std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;

VkImage offscreenImage = VK_NULL_HANDLE;
VkDeviceMemory offscreenMemory = VK_NULL_HANDLE;
VkImageView offscreenImageView = VK_NULL_HANDLE;

VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
VkSemaphore renderingFinishedSemaphore = VK_NULL_HANDLE;
VkFence inFlightFence = VK_NULL_HANDLE; 

VkExtent2D swapChainExtent = {1280, 720};
const VkFormat SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE;
VkSwapchainKHR swapChain = VK_NULL_HANDLE;
std::vector<VkImage> swapChainImages;
VkPipeline pipeline = VK_NULL_HANDLE;
VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
std::vector<VkCommandBuffer> graphicsCommandBuffers;


VkShaderModule CreateShaderModule(const ShaderBinary& InShaderResource) {
    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = InShaderResource.GetSizeInBytes();
    createInfo.pCode = (uint32_t*)InShaderResource.GetData();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        RT_ERROR("failed to create shader module"); exit(1);
    }
    return shaderModule;
}

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    RT_ERROR("failed to find suitable memory type!"); exit(1);
}

void CreateAndUploadBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data, VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBuffer stagingBuffer; VkDeviceMemory stagingMemory;
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size, .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT };
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        RT_ERROR("Failed to create staging buffer"); exit(1);
    }

    VkMemoryRequirements memReqs; 
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);
    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        RT_ERROR("Failed to allocate staging memory"); exit(1);
    }
    
    void* mapped;
    vkMapMemory(device, stagingMemory, 0, size, 0, &mapped);
    memcpy(mapped, data, (size_t)size);
    vkUnmapMemory(device, stagingMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        RT_ERROR("Failed to create GPU buffer"); exit(1);
    }
    
    vkGetBufferMemoryRequirements(device, buffer, &memReqs);
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        RT_ERROR("Failed to allocate GPU memory"); exit(1);
    }
    vkBindBufferMemory(device, buffer, memory, 0);

    VkCommandBufferAllocateInfo allocCmd = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocCmd.commandPool = commandPool;
    allocCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocCmd.commandBufferCount = 1;
    
    VkCommandBuffer cmd; 
    vkAllocateCommandBuffers(device, &allocCmd, &cmd);
    
    VkCommandBufferBeginInfo begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    
    vkBeginCommandBuffer(cmd, &begin);
    VkBufferCopy copyRegion = { 0, 0, size };
    vkCmdCopyBuffer(cmd, stagingBuffer, buffer, 1, &copyRegion);
    vkEndCommandBuffer(cmd);
    
    VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };
    
    vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

void VulkanAPI::SetupVulkan() {
    VulkanAPI::CreateInstance();
    VulkanAPI::CreateWindowSurface();
    VulkanAPI::CreateLogicalDevice();
    VulkanAPI::CreateCommandPool();
    VulkanAPI::CreateSwapChain();
    VulkanAPI::CreateOffscreenResources();
    Scene::CreateGPUBuffers();
    VulkanAPI::CreateDescriptorPool();
    VulkanAPI::CreateDescriptorSet();
    VulkanAPI::CreateComputePipeline();
    VulkanAPI::CreateCommandBuffers();
}

void VulkanAPI::CreateInstance() {
    uint32_t count; 
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + count);
    
    #if defined(__APPLE__)
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back("VK_KHR_get_physical_device_properties2");
    #endif

    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO, .pApplicationName = "TraceyRT", .apiVersion = VK_API_VERSION_1_3 };

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = (uint32_t)extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    #if defined(__APPLE__)
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) { 
        RT_ERROR("failed to create instance"); exit(1); 
    }
}

void VulkanAPI::CreateWindowSurface() {
    if (glfwCreateWindowSurface(instance, Window::GetGLFWwindow(), nullptr, &windowSurface) != VK_SUCCESS) { 
        RT_ERROR("failed to create surface"); exit(1); 
    }
}

void VulkanAPI::CreateLogicalDevice() {
    uint32_t count; 
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count); 
    vkEnumeratePhysicalDevices(instance, &count, devices.data());
    
    physicalDevice = devices[0]; 
    RT_INFO("Using Physical Device Index 0");

    uint32_t qCount; 
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> queues(qCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qCount, queues.data());

    int found = -1;
    for (int i = 0; i < qCount; i++) {
        VkBool32 present = false; 
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &present);
        if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
            found = i; 
            break;
        }
    }
    
    if (found == -1) {
        RT_ERROR("No graphics+present queue found"); exit(1);
    }
    graphicsQueueFamily = presentQueueFamily = found;

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = graphicsQueueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .dynamicRendering = VK_TRUE };

    const char* ext = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceCreateInfo.pNext = &features13;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = &ext;

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) { 
        RT_ERROR("failed to create device"); exit(1); 
    }
    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
}

void VulkanAPI::CreateCommandPool() {
    VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, .queueFamilyIndex = graphicsQueueFamily };
    
    if (vkCreateCommandPool(device, &info, nullptr, &commandPool) != VK_SUCCESS) {
        RT_ERROR("failed to create command pool"); exit(1);
    }
    
    VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(device, &semInfo, nullptr, &renderingFinishedSemaphore);
    
    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
    vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence);
}


void VulkanAPI::CreateSwapChain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &capabilities);

    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == UINT32_MAX) { 
        auto* win = Window::GetInstance();
        extent = { (uint32_t)win->GetWidth(), (uint32_t)win->GetHeight() };
        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    swapChainExtent = extent;

    uint32_t imageCount = std::max(3u, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0) imageCount = std::min(imageCount, capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = SWAPCHAIN_FORMAT;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        RT_ERROR("failed to create swap chain"); exit(1);
    }

    uint32_t count;
    vkGetSwapchainImagesKHR(device, swapChain, &count, nullptr);
    swapChainImages.resize(count);
    vkGetSwapchainImagesKHR(device, swapChain, &count, swapChainImages.data());
}

void VulkanAPI::CreateDescriptorPool() {
    VkDescriptorPoolSize sizes[] = { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 }, { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 } };
    VkDescriptorPoolCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 10, .poolSizeCount = 2, .pPoolSizes = sizes };
    if (vkCreateDescriptorPool(device, &info, nullptr, &descriptorPool) != VK_SUCCESS) {
        RT_ERROR("failed to create descriptor pool"); exit(1);
    }
}

void VulkanAPI::CreateDescriptorSet() {
    if (descriptorSetLayout == VK_NULL_HANDLE) {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        
        VkDescriptorSetLayoutBinding imageBinding{};
        imageBinding.binding = 255;
        imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        imageBinding.descriptorCount = 1;
        imageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings.push_back(imageBinding);

        // Append existing buffers (Note: Ensure your buffer binding points don't conflict with 255)
        auto buffers = Buffer::GetAllBuffers();
        for (size_t i = 0; i < buffers.size(); i++) {
            VkDescriptorSetLayoutBinding b = {};
            b.binding = buffers[i]->GetBindingPoint(); 
            b.descriptorType = buffers[i]->GetDescriptorType();
            b.descriptorCount = 1;
            b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            bindings.push_back(b);
        }

        VkDescriptorSetLayoutCreateInfo descLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        descLayoutInfo.bindingCount = (uint32_t)bindings.size();
        descLayoutInfo.pBindings = bindings.data();
        if (vkCreateDescriptorSetLayout(device, &descLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            RT_ERROR("failed to create descriptor set layout"); exit(1);
        }
    }

    VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        RT_ERROR("failed to allocate descriptor set"); exit(1);
    }

    VkDescriptorImageInfo imageInfo = { .imageLayout = VK_IMAGE_LAYOUT_GENERAL, .imageView = offscreenImageView };
    VkWriteDescriptorSet imageWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    imageWrite.dstSet = descriptorSet;
    imageWrite.dstBinding = 255;
    imageWrite.dstArrayElement = 0;
    imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageWrite.descriptorCount = 1;
    imageWrite.pImageInfo = &imageInfo;

    std::vector<VkWriteDescriptorSet> writes = { imageWrite };
    auto buffers = Buffer::GetAllBuffers();
    for (size_t i = 0; i < buffers.size(); i++) {
        VkWriteDescriptorSet bw = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        bw.dstSet = descriptorSet;
        bw.dstBinding = buffers[i]->GetBindingPoint();
        bw.descriptorType = buffers[i]->GetDescriptorType();
        bw.descriptorCount = 1;
        bw.pBufferInfo = buffers[i]->GetBufferInfo();
        writes.push_back(bw);
    }
    vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void VulkanAPI::CreateOffscreenResources() {
    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapChainExtent.width;
    imageInfo.extent.height = swapChainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM; // Standard storage format
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT; 
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device, &imageInfo, nullptr, &offscreenImage) != VK_SUCCESS) {
        RT_ERROR("failed to create offscreen image"); exit(1);
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, offscreenImage, &memReqs);

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &offscreenMemory) != VK_SUCCESS) {
        RT_ERROR("failed to allocate offscreen image memory"); exit(1);
    }
    vkBindImageMemory(device, offscreenImage, offscreenMemory, 0);

    VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = offscreenImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &offscreenImageView) != VK_SUCCESS) {
        RT_ERROR("failed to create offscreen image view"); exit(1);
    }

    VkCommandBufferAllocateInfo allocCmd = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocCmd.commandPool = commandPool;
    allocCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocCmd.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &allocCmd, &cmd);

    VkCommandBufferBeginInfo begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    vkBeginCommandBuffer(cmd, &begin);

    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.image = offscreenImage;
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);
    
    VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };
    vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
}

void VulkanAPI::CreateComputePipeline() {
    ClearPipeline();
    
    ShaderBinary compBin("ShaderCache/Raytracer.comp.glsl.spv");
    VkShaderModule compShader = CreateShaderModule(compBin);

    VkPipelineShaderStageCreateInfo shaderStage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module = compShader;
    shaderStage.pName = "main";

    VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 1, .pSetLayouts = &descriptorSetLayout };
    
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        RT_ERROR("failed to create pipeline layout"); exit(1);
    }

    VkComputePipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, .stage = shaderStage, .layout = pipelineLayout };

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        RT_ERROR("failed to create compute pipeline"); exit(1);
    }

    vkDestroyShaderModule(device, compShader, nullptr);
}

void VulkanAPI::CreateCommandBuffers() {
    graphicsCommandBuffers.resize(swapChainImages.size());
    
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)graphicsCommandBuffers.size();
    
    if (vkAllocateCommandBuffers(device, &allocInfo, graphicsCommandBuffers.data()) != VK_SUCCESS) {
        RT_ERROR("failed to allocate command buffers"); exit(1);
    }

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    VkClearValue clearValue = { { {0.1f, 0.1f, 0.1f, 1.0f} } };

    for (size_t i = 0; i < graphicsCommandBuffers.size(); i++) {
        VkCommandBuffer cmd = graphicsCommandBuffers[i];
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; 
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT; 
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.image = offscreenImage;
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        
        vkCmdDispatch(cmd, (swapChainExtent.width + 15) / 16, (swapChainExtent.height + 15) / 16, 1);

        VkImageMemoryBarrier barriers[2] = {};
        
        barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barriers[0].image = offscreenImage;
        barriers[0].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[1].srcAccessMask = 0;
        barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[1].image = swapChainImages[i];
        barriers[1].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);

        VkImageBlit blitRegion = {};
        blitRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        blitRegion.srcOffsets[1] = { (int32_t)swapChainExtent.width, (int32_t)swapChainExtent.height, 1 };
        blitRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        blitRegion.dstOffsets[1] = { (int32_t)swapChainExtent.width, (int32_t)swapChainExtent.height, 1 };

        vkCmdBlitImage(cmd, offscreenImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);

        VkImageMemoryBarrier presentBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        presentBarrier.dstAccessMask = 0;
        presentBarrier.image = swapChainImages[i];
        presentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentBarrier);
        vkEndCommandBuffer(cmd);
    }
}

void VulkanAPI::OnWindowSizeChanged() {
    Window::GetInstance()->SetResizedFlag(false);
    Refresh();
}

void VulkanAPI::Draw() {
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) { 
        VulkanAPI::OnWindowSizeChanged(); return; 
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        RT_ERROR("failed to acquire image"); exit(1);
    }
    
    vkResetFences(device, 1, &inFlightFence);

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsCommandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
        RT_ERROR("failed to submit draw command"); exit(1); 
    }

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(graphicsQueue, &presentInfo); 
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || Window::GetInstance()->WasWindowResized()) {
        VulkanAPI::OnWindowSizeChanged();
    }
}

void VulkanAPI::ClearSwapChain() {    
    if (swapChain) {
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
    }
}

void VulkanAPI::ClearCommandBuffers() {
    if (!graphicsCommandBuffers.empty()) {
        vkFreeCommandBuffers(device, commandPool, (uint32_t)graphicsCommandBuffers.size(), graphicsCommandBuffers.data());
        graphicsCommandBuffers.clear();
    }
}

void VulkanAPI::ClearPipeline() {
    if (pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
}

void VulkanAPI::ClearOffscreenResources() {
    vkDestroyImageView(device, offscreenImageView, nullptr);
    vkDestroyImage(device, offscreenImage, nullptr);
    vkFreeMemory(device, offscreenMemory, nullptr);
}

void VulkanAPI::FullCleanUp() {
    vkDeviceWaitIdle(device);
    ClearSwapChain();
    ClearCommandBuffers();
    ClearPipeline();

    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
    
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderingFinishedSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    
    Buffer::DestroyAllBuffers();
    
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, windowSurface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void VulkanAPI::Refresh() {
    vkDeviceWaitIdle(device);
    ClearSwapChain();
    ClearOffscreenResources();
    ClearCommandBuffers();

    CreateSwapChain();
    CreateOffscreenResources();
    CreateDescriptorSet();
    CreateComputePipeline();
    CreateCommandBuffers(); 
}
