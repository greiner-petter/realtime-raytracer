#include "VulkanAPI.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <functional>
#include <cassert>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "common/Window.h"

#include "common/Log.h"
#include "common/common.h"
#include "common/Types.h"

#include "vulkan/Buffer.h"
#include "vulkan/GraphicsPipeline.h"
std::shared_ptr<GraphicsPipeline> graphicsPipeline;

struct Vertex {
    Vec3 Position;
    Vec3 Color;
};



#if defined(__APPLE__)
#include <vulkan/vulkan_macos.h>
#endif


const bool ENABLE_DEBUGGING = false;

const char* DEBUG_LAYER = "VK_LAYER_LUNARG_standard_validation";

// Debug callback
VkBool32 debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData) {
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        RT_ERROR("ERROR: [{0}] Code {1} : {2}", pLayerPrefix, msgCode, pMsg);
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        RT_WARN("WARNING: [{0}] Code {1} : {2}", pLayerPrefix, msgCode, pMsg);
	}

	//exit(1);

	return VK_FALSE;
}


static VkInstance instance;
VkSurfaceKHR windowSurface;
VkPhysicalDevice physicalDevice;
VkDevice device;
static VkDebugReportCallbackEXT callback;
static VkQueue graphicsQueue;
static VkQueue presentQueue;
static VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
static VkSemaphore imageAvailableSemaphore;
static VkSemaphore renderingFinishedSemaphore;
 
VkBuffer vertexBuffer;
static VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
static VkDeviceMemory indexBufferMemory;
VkVertexInputBindingDescription vertexBindingDescription;
std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;

UBO uniformBufferData;
std::shared_ptr<UniformBuffer> uniformBuffer;
VkDescriptorSetLayout descriptorSetLayout;
static VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;

#include "scene/Sphere.h"
#include "scene/Scene.h"
 


VkCommandPool commandPool;
uint32_t graphicsQueueFamily;
uint32_t presentQueueFamily;

// Find device memory that is supported by the requirements (typeBits) and meets the desired properties
VkBool32 GetMemoryType(uint32_t typeBits, VkFlags properties, uint32_t* typeIndex) {
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    return false;
}



std::shared_ptr<SSBO> sceneSSBO;

void VulkanAPI::SetupVulkan() {
    VulkanAPI::CreateInstance();
    VulkanAPI::CreateDebugCallback();
    VulkanAPI::CreateWindowSurface();
    VulkanAPI::FindPhysicalDevice();
    VulkanAPI::CheckSwapChainSupport();
    VulkanAPI::FindQueueFamilies();
    VulkanAPI::CreateLogicalDevice();
    VulkanAPI::CreateSemaphores();
    VulkanAPI::CreateCommandPool();
    graphicsPipeline = GraphicsPipeline::Create();
    graphicsPipeline->oldSwapChain = VK_NULL_HANDLE;
    graphicsPipeline->CreateSwapChain();
    graphicsPipeline->CreateRenderPass();
    VulkanAPI::CreateVertexBuffer();
    VulkanAPI::CreateUniformBuffer();
    sceneSSBO = SSBO::Create(1);
    graphicsPipeline->CreateImageViews();
    graphicsPipeline->CreateFramebuffers();
    graphicsPipeline->CreateGraphicsPipeline();
    VulkanAPI::CreateDescriptorPool();
    VulkanAPI::CreateDescriptorSet();
    graphicsPipeline->CreateCommandBuffers();
}

void VulkanAPI::CleanUp(bool fullClean) {
    graphicsPipeline->PartialCleanUp();

    if (fullClean) {
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device, renderingFinishedSemaphore, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);

        // Clean up uniform buffer related objects
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        Buffer::DestroyAllBuffers();

        // Buffers must be destroyed after no command buffers are referring to them anymore
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        // Note: implicitly destroys images (in fact, we're not allowed to do that explicitly)
        vkDestroySwapchainKHR(device, graphicsPipeline->swapChain, nullptr);

        vkDestroyDevice(device, nullptr);

        vkDestroySurfaceKHR(instance, windowSurface, nullptr);

        if (ENABLE_DEBUGGING) {
            PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
            DestroyDebugReportCallback(instance, callback, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
    }
}

void VulkanAPI::CreateInstance() {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkanClear";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "ClearScreenEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    // Get instance extensions required by GLFW to draw to window
    unsigned int glfwExtensionCount;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions;
    for (size_t i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }
    
#if defined(__APPLE__)
    extensions.push_back("VK_KHR_portability_enumeration");
#endif

    if (ENABLE_DEBUGGING) {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    // Check for extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    if (extensionCount == 0) {
        RT_ERROR("no extensions supported!");
        exit(1);
    }

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    RT_INFO("supported extensions:");

    for (const auto& extension : availableExtensions) {
        RT_INFO("- {0}", extension.extensionName);
    }

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = (uint32_t)extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
#if defined(__APPLE__)
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    if (ENABLE_DEBUGGING) {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &DEBUG_LAYER;
    }

    // Initialize Vulkan instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        RT_ERROR("failed to create instance!");
        exit(1);
    }
    else {
        RT_INFO("created vulkan instance");
    }
}

void VulkanAPI::CreateWindowSurface() {
    if (glfwCreateWindowSurface(instance, Window::GetGLFWwindow(), NULL, &windowSurface) != VK_SUCCESS) {
        RT_ERROR("failed to create window surface!");
        exit(1);
    }

    RT_INFO("created window surface");
}

void VulkanAPI::FindPhysicalDevice() {
    // Try to find 1 Vulkan supported device
    // Note: perhaps refactor to loop through devices and find first one that supports all required features and extensions
    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) {
        RT_ERROR("failed to get number of physical devices");
        exit(1);
    }

    deviceCount = 1;
    VkResult res = vkEnumeratePhysicalDevices(instance, &deviceCount, &physicalDevice);
    if (res != VK_SUCCESS && res != VK_INCOMPLETE) {
        RT_ERROR("enumerating physical devices failed!");
        exit(1);
    }

    if (deviceCount == 0) {
        RT_ERROR("no physical devices that support vulkan!");
        exit(1);
    }

    RT_INFO("physical device with vulkan support found");

    // Check device features
    // Note: will apiVersion >= appInfo.apiVersion? Probably yes, but spec is unclear.
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    uint32_t supportedVersion[] = {
        VK_VERSION_MAJOR(deviceProperties.apiVersion),
        VK_VERSION_MINOR(deviceProperties.apiVersion),
        VK_VERSION_PATCH(deviceProperties.apiVersion)
    };

    RT_INFO("physical device supports version {0}.{1}.{2}", supportedVersion[0], supportedVersion[1], supportedVersion[2]);
}

void VulkanAPI::CheckSwapChainSupport() {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    if (extensionCount == 0) {
        RT_ERROR("physical device doesn't support any extensions");
        exit(1);
    }

    std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, deviceExtensions.data());

    for (const auto& extension : deviceExtensions) {
        if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            RT_INFO("physical device supports swap chains");
            return;
        }
    }

    RT_ERROR("physical device doesn't support swap chains");
    exit(1);
}

void VulkanAPI::FindQueueFamilies() {
    // Check queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    if (queueFamilyCount == 0) {
        RT_INFO("physical device has no queue families!");
        exit(1);
    }

    // Find queue family with graphics support
    // Note: is a transfer queue necessary to copy vertices to the gpu or can a graphics queue handle that?
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    RT_INFO("physical device has {0} queue families", queueFamilyCount);

    bool foundGraphicsQueueFamily = false;
    bool foundPresentQueueFamily = false;

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &presentSupport);

        if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamily = i;
            foundGraphicsQueueFamily = true;

            if (presentSupport) {
                presentQueueFamily = i;
                foundPresentQueueFamily = true;
                break;
            }
        }

        if (!foundPresentQueueFamily && presentSupport) {
            presentQueueFamily = i;
            foundPresentQueueFamily = true;
        }
    }

    if (foundGraphicsQueueFamily) {
        RT_INFO("queue family #{0} supports graphics", graphicsQueueFamily);

        if (foundPresentQueueFamily) {
            RT_INFO("queue family #{0} supports presentation", presentQueueFamily);
        }
        else {
            RT_ERROR("could not find a valid queue family with present support");
            exit(1);
        }
    }
    else {
        RT_ERROR("could not find a valid queue family with graphics support");
        exit(1);
    }
}

void VulkanAPI::CreateLogicalDevice() {
    // Greate one graphics queue and optionally a separate presentation queue
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo[2] = {};

    queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamily;
    queueCreateInfo[0].queueCount = 1;
    queueCreateInfo[0].pQueuePriorities = &queuePriority;

    queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[0].queueFamilyIndex = presentQueueFamily;
    queueCreateInfo[0].queueCount = 1;
    queueCreateInfo[0].pQueuePriorities = &queuePriority;

    // Create logical device from physical device
    // Note: there are separate instance and device extensions!
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;

    if (graphicsQueueFamily == presentQueueFamily) {
        deviceCreateInfo.queueCreateInfoCount = 1;
    }
    else {
        deviceCreateInfo.queueCreateInfoCount = 2;
    }

    // Necessary for shader (for some reason)
    VkPhysicalDeviceFeatures enabledFeatures = {};
#ifndef __APPLE__
    enabledFeatures.shaderClipDistance = VK_TRUE;
    enabledFeatures.shaderCullDistance = VK_TRUE;
#endif

    const char* deviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensions;
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

    if (ENABLE_DEBUGGING) {
        deviceCreateInfo.enabledLayerCount = 1;
        deviceCreateInfo.ppEnabledLayerNames = &DEBUG_LAYER;
    }

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        RT_ERROR("failed to create logical device");
        exit(1);
    }

    RT_INFO("created logical device");

    // Get graphics and presentation queues (which may be the same)
    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamily, 0, &presentQueue);

    RT_INFO("acquired graphics and presentation queues");

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
}

void VulkanAPI::CreateDebugCallback() {
    if (ENABLE_DEBUGGING) {
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugCallback;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

        PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

        if (CreateDebugReportCallback(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
            RT_ERROR("failed to create debug callback");
            exit(1);
        }
        else {
            RT_INFO("created debug callback");
        }
    }
    else {
        RT_INFO("skipped creating debug callback");
    }
}

void VulkanAPI::CreateSemaphores() {
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device, &createInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &createInfo, nullptr, &renderingFinishedSemaphore) != VK_SUCCESS) {
        RT_ERROR("failed to create semaphores");
        exit(1);
    }
    else {
        RT_INFO("created semaphores");
    }
}

void VulkanAPI::CreateCommandPool() {
    // Create graphics command pool
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = graphicsQueueFamily;

    if (vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        RT_ERROR("failed to create command queue for graphics queue family");
        exit(1);
    }
    else {
        RT_INFO("created command pool for graphics queue family");
    }
}

void VulkanAPI::CreateVertexBuffer() {
    // Setup vertices
    std::vector<Vertex> vertices = {
        { Vec3{ -1.0f, -1.0f,  0.0f }, {} },
        { Vec3{ -1.0f,  1.0f,  0.0f }, {} },
        { Vec3{  1.0f,  1.0f,  0.0f }, {} },
        { Vec3{  1.0f, -1.0f,  0.0f }, {} },
    };
    uint32_t verticesSize = (uint32_t)(vertices.size() * sizeof(vertices[0]));

    // Setup indices
    std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };
    uint32_t indicesSize = (uint32_t)(indices.size() * sizeof(indices[0]));

    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;
    void* data;

    struct StagingBuffer {
        VkDeviceMemory memory;
        VkBuffer buffer;
    };

    struct {
        StagingBuffer vertices;
        StagingBuffer indices;
    } stagingBuffers;

    // Allocate command buffer for copy operation
    VkCommandBufferAllocateInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufInfo.commandPool = commandPool;
    cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufInfo.commandBufferCount = 1;

    VkCommandBuffer copyCommandBuffer;
    vkAllocateCommandBuffers(device, &cmdBufInfo, &copyCommandBuffer);

    // First copy vertices to host accessible vertex buffer memory
    VkBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = verticesSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    vkCreateBuffer(device, &vertexBufferInfo, nullptr, &stagingBuffers.vertices.buffer);

    vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory);

    vkMapMemory(device, stagingBuffers.vertices.memory, 0, verticesSize, 0, &data);
    memcpy(data, vertices.data(), verticesSize);
    vkUnmapMemory(device, stagingBuffers.vertices.memory);
    vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0);

    // Then allocate a gpu only buffer for vertices
    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(device, &vertexBufferInfo, nullptr, &vertexBuffer);
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(device, &memAlloc, nullptr, &vertexBufferMemory);
    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

    // Next copy indices to host accessible index buffer memory
    VkBufferCreateInfo indexBufferInfo = {};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = indicesSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    vkCreateBuffer(device, &indexBufferInfo, nullptr, &stagingBuffers.indices.buffer);
    vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.indices.memory);
    vkMapMemory(device, stagingBuffers.indices.memory, 0, indicesSize, 0, &data);
    memcpy(data, indices.data(), indicesSize);
    vkUnmapMemory(device, stagingBuffers.indices.memory);
    vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0);

    // And allocate another gpu only buffer for indices
    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(device, &indexBufferInfo, nullptr, &indexBuffer);
    vkGetBufferMemoryRequirements(device, indexBuffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(device, &memAlloc, nullptr, &indexBufferMemory);
    vkBindBufferMemory(device, indexBuffer, indexBufferMemory, 0);

    // Now copy data from host visible buffer to gpu only buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(copyCommandBuffer, &bufferBeginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.size = verticesSize;
    vkCmdCopyBuffer(copyCommandBuffer, stagingBuffers.vertices.buffer, vertexBuffer, 1, &copyRegion);
    copyRegion.size = indicesSize;
    vkCmdCopyBuffer(copyCommandBuffer, stagingBuffers.indices.buffer, indexBuffer, 1, &copyRegion);

    vkEndCommandBuffer(copyCommandBuffer);

    // Submit to queue
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCommandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &copyCommandBuffer);

    vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
    vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
    vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
    vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);

    RT_INFO("set up vertex and index buffers");

    // Binding and attribute descriptions
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = sizeof(vertices[0]);
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // vec2 position
    vertexAttributeDescriptions.resize(2);
    vertexAttributeDescriptions[0].binding = 0;
    vertexAttributeDescriptions[0].location = 0;
    vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;

    // vec3 color
    vertexAttributeDescriptions[1].binding = 0;
    vertexAttributeDescriptions[1].location = 1;
    vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[1].offset = sizeof(float) * 3;
}

void VulkanAPI::CreateUniformBuffer() {
    uniformBuffer = UniformBuffer::Create(0, sizeof(uniformBufferData));
    VulkanAPI::UpdateUniformData();
}

void VulkanAPI::UpdateSceneData(Scene& scene) {
    scene.ConvertSceneToGPUData();
    sceneSSBO->UploadData(scene.GetGPUData(), scene.GetGPUDataSize());
}

void VulkanAPI::UpdateUniformData() {
    uniformBufferData.u_resolution = glm::vec2(Window::GetInstance()->GetWidth(), Window::GetInstance()->GetHeight());
    uniformBufferData.u_aspectRatio = float(Window::GetInstance()->GetHeight()) / float(Window::GetInstance()->GetWidth());

    uniformBuffer->UploadData(&uniformBufferData, sizeof(uniformBufferData));
}


void VulkanAPI::CreateDescriptorPool() {
    VkDescriptorPoolSize poolSizes[2]{};

    // UBO (binding = 0)
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;

    // SSBO (binding = 1)
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 2;
    createInfo.pPoolSizes = poolSizes;
    createInfo.maxSets = 1;

    if (vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        RT_ERROR("failed to create descriptor pool");
        std::exit(1);
    }

    RT_INFO("created descriptor pool");
}

void VulkanAPI::CreateDescriptorSet() {
    // There needs to be one descriptor set per binding point in the shader
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    if (descriptorSet != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
    }

    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        RT_ERROR("failed to create descriptor set");
        exit(1);
    } else {
        RT_INFO("created descriptor set");
    }
    
    auto buffers = Buffer::GetAllBuffers();
    std::vector<VkWriteDescriptorSet> writes(buffers.size());

    for (size_t i = 0; i < buffers.size(); i++) {
        writes[i] = {};
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = descriptorSet;
        writes[i].dstBinding = buffers[i]->GetBindingPoint();
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = buffers[i]->GetDescriptorType();
        writes[i].pBufferInfo = buffers[i]->GetBufferInfo();
        RT_ASSERT(buffers[i]->GetBufferInfo() != nullptr, "Buffer info for buffer at binding point {0} is null!", buffers[i]->GetBindingPoint());
        RT_ASSERT(buffers[i]->GetBufferInfo()->buffer != VK_NULL_HANDLE, "Buffer for buffer at binding point {0} is null!", buffers[i]->GetBindingPoint());
        RT_ASSERT(buffers[i]->GetBufferInfo()->range > 0, "Buffer range for buffer at binding point {0} is zero!", buffers[i]->GetBindingPoint());
        RT_ASSERT(buffers[i]->GetBufferInfo()->offset >= 0, "Buffer offset for buffer at binding point {0} is negative!", buffers[i]->GetBindingPoint());
        writes[i].dstArrayElement = 0;
        assert(buffers[i]->GetBuffer() != VK_NULL_HANDLE);
    }

    vkUpdateDescriptorSets(device, buffers.size(), writes.data(), 0, nullptr);
}

void VulkanAPI::OnWindowSizeChanged() {
    //windowResized = false;
    Window::GetInstance()->SetResizedFlag(false);

    graphicsPipeline->Refresh();
    /*
    // Only recreate objects that are affected by framebuffer size changes
    VulkanAPI::CleanUp(false);

    VulkanAPI::CreateSwapChain();
    VulkanAPI::CreateRenderPass();
    VulkanAPI::CreateImageViews();
    VulkanAPI::CreateFramebuffers();
    VulkanAPI::CreateDescriptorSet(); // TODO: buffer info may have changed
    VulkanAPI::CreateGraphicsPipeline();
    VulkanAPI::CreateCommandBuffers();
    */
}

void VulkanAPI::Draw() {
    // Acquire image
    uint32_t imageIndex;
    VkResult res = vkAcquireNextImageKHR(device, graphicsPipeline->swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // Unless surface is out of date right now, defer swap chain recreation until end of this frame
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        RT_TRACE("swapchain image fault {0}", (int64_t) res);
        VulkanAPI::OnWindowSizeChanged();
        return;
    }
    else if (res != VK_SUCCESS) {
        RT_ERROR("failed to acquire image");
        exit(1);
    }

    // Wait for image to be available and draw
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

    // This is the stage where the queue should wait on the semaphore
    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsPipeline->graphicsCommandBuffers[imageIndex];

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        RT_ERROR("failed to submit draw command buffer");
        exit(1);
    }

    // Present drawn image
    // Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &graphicsPipeline->swapChain;
    presentInfo.pImageIndices = &imageIndex;

    res = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR || Window::GetInstance()->WasWindowResized()) {
        VulkanAPI::OnWindowSizeChanged();
    }
    else if (res != VK_SUCCESS) {
        RT_ERROR("failed to submit present command buffer");
        exit(1);
    }
}
