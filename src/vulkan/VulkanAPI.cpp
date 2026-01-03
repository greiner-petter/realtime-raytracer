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

VkBuffer vertexBuffer = VK_NULL_HANDLE;
VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
VkBuffer indexBuffer = VK_NULL_HANDLE;
VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
VkVertexInputBindingDescription vertexBindingDescription = {};
std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;

VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
VkSemaphore renderingFinishedSemaphore = VK_NULL_HANDLE;
VkFence inFlightFence = VK_NULL_HANDLE; 

VkExtent2D swapChainExtent = {1280, 720};
const VkFormat SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE;
VkSwapchainKHR swapChain = VK_NULL_HANDLE;
std::vector<VkImage> swapChainImages;
std::vector<VkImageView> swapChainImageViews;
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

void VulkanAPI::SetupVulkan() {
    VulkanAPI::CreateInstance();
    VulkanAPI::CreateWindowSurface();
    VulkanAPI::CreateLogicalDevice();
    VulkanAPI::CreateCommandPool();
    VulkanAPI::CreateSwapChain();
    VulkanAPI::CreateImageViews();
    VulkanAPI::CreateVertexBuffer();
    Scene::CreateGPUBuffers();
    VulkanAPI::CreateDescriptorPool();
    VulkanAPI::CreateDescriptorSet();
    VulkanAPI::CreateGraphicsPipeline();
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
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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

void VulkanAPI::CreateImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = SWAPCHAIN_FORMAT;
        createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
             RT_ERROR("failed to create image view"); exit(1);
        }
    }
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


void VulkanAPI::CreateGraphicsPipeline() {
    ClearPipeline();

    VkPipelineRenderingCreateInfo renderingInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO }; 
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &SWAPCHAIN_FORMAT;
    renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    ShaderBinary vertBin("ShaderCache/Shader_Vert.vert.glsl.spv");
    ShaderBinary fragBin("ShaderCache/Shader_Frag.frag.glsl.spv");
    VkShaderModule vert = CreateShaderModule(vertBin);
    VkShaderModule frag = CreateShaderModule(fragBin);

    VkPipelineShaderStageCreateInfo shaderStages[2] = {};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vert;
    shaderStages[0].pName = "main";
    
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = frag;
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_TRUE; // Enable blending
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;               // Src color * src alpha
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;     // Dst color * (1 - src alpha)
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;                               // Add the results
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;                    // Src alpha * 1
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;                   // Dst alpha * 0
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;                              // Add the results
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT 
                                       | VK_COLOR_COMPONENT_G_BIT 
                                       | VK_COLOR_COMPONENT_B_BIT 
                                       | VK_COLOR_COMPONENT_A_BIT;  // Write all channels


    VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

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
        
        if (vkCreateDescriptorSetLayout(device, &descLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            RT_ERROR("failed to create descriptor set layout"); exit(1);
        }
    }

    VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        RT_ERROR("failed to create pipeline layout"); exit(1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.pTessellationState = nullptr; 

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        RT_ERROR("failed to create pipeline"); exit(1);
    }

    vkDestroyShaderModule(device, vert, nullptr);
    vkDestroyShaderModule(device, frag, nullptr);
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
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.image = swapChainImages[i];
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkRenderingAttachmentInfo colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = swapChainImageViews[i];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearValue;

        VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea = { {0, 0}, swapChainExtent };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport = { 0.0f, 0.0f, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f, 1.0f };
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        
        VkRect2D scissor = { {0, 0}, swapChainExtent };
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
        vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

        vkCmdEndRendering(cmd);

        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0; // VK_ACCESS_MEMORY_READ_BIT not strictly needed for present, but safe
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
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
    for (auto view : swapChainImageViews) {
        vkDestroyImageView(device, view, nullptr);
    }
    swapChainImageViews.clear();
    
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
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
}

void VulkanAPI::FullCleanUp() {
    vkDeviceWaitIdle(device);
    ClearSwapChain();
    ClearCommandBuffers();
    ClearPipeline();
    
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

void VulkanAPI::Refresh() {
    vkDeviceWaitIdle(device);
    ClearSwapChain();
    ClearCommandBuffers();

    CreateSwapChain();
    CreateImageViews();
    CreateGraphicsPipeline();
    CreateCommandBuffers(); 
}
