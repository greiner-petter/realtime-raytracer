#include "VulkanAPI.h"
#include "common/Log.h"
#include "common/Window.h"
#include "common/Types.h"
#include "vulkan/Buffer.h"
#include "vulkan/GraphicsPipeline.h"
#include "scene/Scene.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <cstring>
#include <algorithm>

std::shared_ptr<GraphicsPipeline> graphicsPipeline;
VkInstance instance;
VkSurfaceKHR windowSurface;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkQueue graphicsQueue; 
VkCommandPool commandPool;
uint32_t graphicsQueueFamily;
uint32_t presentQueueFamily; 

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
VkDescriptorSetLayout descriptorSetLayout;
VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;
VkVertexInputBindingDescription vertexBindingDescription;
std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;

VkSemaphore imageAvailableSemaphore;
VkSemaphore renderingFinishedSemaphore;
VkFence inFlightFence; 

VkBool32 GetMemoryType(uint32_t typeBits, VkFlags properties, uint32_t* typeIndex) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            *typeIndex = i;
            return VK_TRUE;
        }
    }
    return VK_FALSE;
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
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) { RT_ERROR("Failed to create staging buffer"); exit(1); }

    VkMemoryRequirements memReqs; vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);
    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) { RT_ERROR("Failed to allocate staging memory"); exit(1); }
    
    void* mapped;
    vkMapMemory(device, stagingMemory, 0, size, 0, &mapped);
    memcpy(mapped, data, (size_t)size);
    vkUnmapMemory(device, stagingMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) { RT_ERROR("Failed to create GPU buffer"); exit(1); }
    
    vkGetBufferMemoryRequirements(device, buffer, &memReqs);
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) { RT_ERROR("Failed to allocate GPU memory"); exit(1); }
    vkBindBufferMemory(device, buffer, memory, 0);

    VkCommandBufferAllocateInfo allocCmd = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocCmd.commandPool = commandPool;
    allocCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocCmd.commandBufferCount = 1;
    
    VkCommandBuffer cmd; 
    vkAllocateCommandBuffers(device, &allocCmd, &cmd);
    
    VkCommandBufferBeginInfo begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(cmd, &begin);
    VkBufferCopy copyRegion = { 0, 0, size };
    vkCmdCopyBuffer(cmd, stagingBuffer, buffer, 1, &copyRegion);
    vkEndCommandBuffer(cmd);
    
    VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    
    vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

std::shared_ptr<Scene> VulkanAPI::SetupVulkan() {
    VulkanAPI::CreateInstance();
    VulkanAPI::CreateWindowSurface();
    VulkanAPI::CreateLogicalDevice(); 
    VulkanAPI::CreateCommandPool(); 

    graphicsPipeline = GraphicsPipeline::Create();
    graphicsPipeline->CreateSwapChain();
    graphicsPipeline->CreateImageViews();
    
    VulkanAPI::CreateVertexBuffer();
    auto scene = std::make_shared<Scene>();
    
    VulkanAPI::CreateDescriptorPool(); 
    VulkanAPI::CreateDescriptorSet(); 
    
    graphicsPipeline->CreateGraphicsPipeline(); 
    graphicsPipeline->CreateCommandBuffers();

    return scene;
}

void VulkanAPI::CreateInstance() {
    uint32_t count; 
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + count);
    
    #if defined(__APPLE__)
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back("VK_KHR_get_physical_device_properties2");
    #endif

    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "TraceyRT";
    appInfo.apiVersion = VK_API_VERSION_1_3; 

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
    
    // Simple Selection: First device
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
            found = i; break;
        }
    }
    
    if (found == -1) { RT_ERROR("No graphics+present queue found"); exit(1); }
    graphicsQueueFamily = presentQueueFamily = found;

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = graphicsQueueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features13.dynamicRendering = VK_TRUE;

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
    VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; 
    info.queueFamilyIndex = graphicsQueueFamily;
    
    if (vkCreateCommandPool(device, &info, nullptr, &commandPool) != VK_SUCCESS) {
        RT_ERROR("failed to create command pool"); exit(1);
    }
    
    VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(device, &semInfo, nullptr, &renderingFinishedSemaphore);
    
    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 
    vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence);
}

void VulkanAPI::CreateVertexBuffer() {
    struct Vertex { Vec3 pos; Vec3 col; };
    std::vector<Vertex> vertices = {
        {{-1.0f, -1.0f, 0.0f}, {}}, {{-1.0f, 1.0f, 0.0f}, {}},
        {{ 1.0f,  1.0f, 0.0f}, {}}, {{ 1.0f, -1.0f, 0.0f}, {}}
    };
    std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

    CreateAndUploadBuffer(sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices.data(), vertexBuffer, vertexBufferMemory);
    CreateAndUploadBuffer(sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices.data(), indexBuffer, indexBufferMemory);

    vertexBindingDescription = { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    vertexAttributeDescriptions = {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) },
        { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, col) }
    };
}

void VulkanAPI::CreateDescriptorPool() {
    VkDescriptorPoolSize sizes[] = { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 }, { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 } };
    VkDescriptorPoolCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    info.maxSets = 10;
    info.poolSizeCount = 2;
    info.pPoolSizes = sizes;
    
    if (vkCreateDescriptorPool(device, &info, nullptr, &descriptorPool) != VK_SUCCESS) {
        RT_ERROR("failed to create descriptor pool"); exit(1);
    }
}

void VulkanAPI::CreateDescriptorSet() {
    if (descriptorSetLayout == VK_NULL_HANDLE) {
        auto buffers = Buffer::GetAllBuffers();
        std::vector<VkDescriptorSetLayoutBinding> bindings(buffers.size());
        for (size_t i = 0; i < buffers.size(); i++) {
            bindings[i] = {};
            bindings[i].binding = buffers[i]->GetBindingPoint();
            bindings[i].descriptorType = buffers[i]->GetDescriptorType();
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        VkDescriptorSetLayoutCreateInfo descLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        descLayoutInfo.bindingCount = (uint32_t)buffers.size();
        descLayoutInfo.pBindings = bindings.data();
        if (vkCreateDescriptorSetLayout(device, &descLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) exit(1);
    }

    VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        RT_ERROR("failed to allocate descriptor set"); exit(1);
    }
    
    auto buffers = Buffer::GetAllBuffers();
    std::vector<VkWriteDescriptorSet> writes(buffers.size());
    for (size_t i = 0; i < buffers.size(); i++) {
        writes[i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writes[i].dstSet = descriptorSet;
        writes[i].dstBinding = buffers[i]->GetBindingPoint();
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = buffers[i]->GetDescriptorType();
        writes[i].pBufferInfo = buffers[i]->GetBufferInfo();
    }
    vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void VulkanAPI::OnWindowSizeChanged() {
    Window::GetInstance()->SetResizedFlag(false);
    graphicsPipeline->Refresh();
}

void VulkanAPI::Draw() {
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, graphicsPipeline->swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) { 
        VulkanAPI::OnWindowSizeChanged(); return; 
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        RT_ERROR("failed to acquire image"); exit(1);
    }
    
    vkResetFences(device, 1, &inFlightFence);

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsPipeline->graphicsCommandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) { 
        RT_ERROR("failed to submit draw command"); exit(1); 
    }

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &graphicsPipeline->swapChain;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(graphicsQueue, &presentInfo); 
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || Window::GetInstance()->WasWindowResized()) {
        VulkanAPI::OnWindowSizeChanged();
    }
}

void VulkanAPI::CleanUp(bool fullClean) {
    vkDeviceWaitIdle(device);
    graphicsPipeline->ClearPipeline();
    graphicsPipeline->ClearSwapChain();
    graphicsPipeline->ClearCommandBuffers();
    
    if (fullClean) {
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device, renderingFinishedSemaphore, nullptr);
        vkDestroyFence(device, inFlightFence, nullptr);
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        
        Buffer::DestroyAllBuffers();

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);
        
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, windowSurface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
}
