#include "GraphicsPipeline.h"

#include "common/Log.h"
#include "common/Window.h"
#include "vulkan/Buffer.h"
#include "vulkan/VulkanAPI.h"

#include "Shader_Vert.h"
#include "Shader_Frag.h"

extern VkDevice device;
extern VkDescriptorSetLayout descriptorSetLayout;
extern uint32_t graphicsQueueFamily;
extern uint32_t presentQueueFamily;
extern VkSurfaceKHR windowSurface;
extern VkPhysicalDevice physicalDevice;
extern VkVertexInputBindingDescription vertexBindingDescription;
extern std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
extern VkCommandPool commandPool;

extern VkBuffer vertexBuffer;
extern VkBuffer indexBuffer;
extern VkDescriptorSet descriptorSet;

std::shared_ptr<GraphicsPipeline> GraphicsPipeline::Create() {
    auto graphicsPipeline = std::make_shared<GraphicsPipeline>();
    return graphicsPipeline;
}

VkShaderModule CreateShaderModule(const Resource& InShaderResource) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = InShaderResource.m_SizeInBytes;
    createInfo.pCode = (uint32_t*)InShaderResource.m_Data;

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        RT_ERROR("failed to create shader module");
        exit(1);
    }

    return shaderModule;
}

VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> presentModes) {
    for (const auto& presentMode : presentModes) {
        if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            return presentMode;
        }
    }
    for (const auto& presentMode : presentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    // If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
    return VK_PRESENT_MODE_FIFO_KHR;
}

void GraphicsPipeline::Refresh() {
    PartialCleanUp();

    CreateSwapChain();
    CreateRenderPass();
    CreateImageViews();
    CreateFramebuffers();
    VulkanAPI::CreateDescriptorSet(); // TODO: buffer info may have changed
    CreateGraphicsPipeline();
    CreateCommandBuffers();
}

void GraphicsPipeline::PartialCleanUp() {
    vkDeviceWaitIdle(device);

    vkFreeCommandBuffers(device, commandPool, (uint32_t)graphicsCommandBuffers.size(), graphicsCommandBuffers.data());

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // We can either choose any format
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    }

    // Or go with the standard format - if available
    for (const auto& availableSurfaceFormat : availableFormats) {
        if (availableSurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM) {
            return availableSurfaceFormat;
        }
    }

    // Or fall back to the first available one
    return availableFormats[0];
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
    if (surfaceCapabilities.currentExtent.width == -1) {
        VkExtent2D swapChainExtent = {};

        swapChainExtent.width = std::min(std::max(Window::GetInstance()->GetWidth(), surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width);
        swapChainExtent.height = std::min(std::max(Window::GetInstance()->GetHeight(), surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height);
        
        return swapChainExtent;
    }
    else {
        return surfaceCapabilities.currentExtent;
    }
}

void GraphicsPipeline::CreateSwapChain() {
    // Find surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities) != VK_SUCCESS) {
        RT_ERROR("failed to acquire presentation surface capabilities");
        exit(1);
    }

    // Find supported surface formats
    uint32_t formatCount;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0) {
        RT_ERROR("failed to get number of supported surface formats");
        exit(1);
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, surfaceFormats.data()) != VK_SUCCESS) {
        RT_ERROR("failed to get supported surface formats");
        exit(1);
    }

    // Find supported present modes
    uint32_t presentModeCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0) {
        RT_ERROR("failed to get number of supported presentation modes");
        exit(1);
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, presentModes.data()) != VK_SUCCESS) {
        RT_ERROR("failed to get supported presentation modes");
        exit(1);
    }

    // Determine number of images for swap chain
    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    RT_INFO("using {0} images for swap chain", imageCount);

    // Select a surface format
    VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(surfaceFormats);

    // Select swap chain size
    swapChainExtent = ChooseSwapExtent(surfaceCapabilities);

    // Determine transformation to use (preferring no transform)
    VkSurfaceTransformFlagBitsKHR surfaceTransform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        surfaceTransform = surfaceCapabilities.currentTransform;
    }

    // Choose presentation mode (preferring MAILBOX ~= triple buffering)
    VkPresentModeKHR presentMode = ChoosePresentMode(presentModes);
    RT_INFO("using presentation mode: {0}", presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR ? "IMMEDIATE" : (presentMode == VK_PRESENT_MODE_MAILBOX_KHR ? "MAILBOX" : "FIFO"));

    // Finally, create the swap chain
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = surfaceTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapChain;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        RT_ERROR("failed to create swap chain");
        exit(1);
    }
    else {
        RT_INFO("created swap chain");
    }

    if (oldSwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, oldSwapChain, nullptr);
    }
    oldSwapChain = swapChain;

    swapChainFormat = surfaceFormat.format;

    // Store the images used by the swap chain
    // Note: these are the images that swap chain image indices refer to
    // Note: actual number of images may differ from requested number, since it's a lower bound
    uint32_t actualImageCount = 0;
    if (vkGetSwapchainImagesKHR(device, swapChain, &actualImageCount, nullptr) != VK_SUCCESS || actualImageCount == 0) {
        RT_ERROR("failed to acquire number of swap chain images");
        exit(1);
    }

    swapChainImages.resize(actualImageCount);

    if (vkGetSwapchainImagesKHR(device, swapChain, &actualImageCount, swapChainImages.data()) != VK_SUCCESS) {
        RT_ERROR("failed to acquire swap chain images");
        exit(1);
    }

    RT_INFO("acquired swap chain images");
}

void GraphicsPipeline::CreateRenderPass() {
    VkAttachmentDescription attachmentDescription = {};
    attachmentDescription.format = swapChainFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Note: hardware will automatically transition attachment to the specified layout
    // Note: index refers to attachment descriptions array
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Note: this is a description of how the attachments of the render pass will be used in this sub pass
    // e.g. if they will be read in shaders and/or drawn to
    VkSubpassDescription subPassDescription = {};
    subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPassDescription.colorAttachmentCount = 1;
    subPassDescription.pColorAttachments = &colorAttachmentReference;

    // Create the render pass
    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &attachmentDescription;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subPassDescription;

    if (vkCreateRenderPass(device, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
        RT_ERROR("failed to create render pass");
        exit(1);
    }
    else {
        RT_INFO("created render pass");
    }
}


void GraphicsPipeline::CreateGraphicsPipeline() {
    VkShaderModule vertexShaderModule = CreateShaderModule(Resources::SHADER_VERT);
    VkShaderModule fragmentShaderModule = CreateShaderModule(Resources::SHADER_FRAG);

    // Set up shader stage info
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCreateInfo.module = vertexShaderModule;
    vertexShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderCreateInfo.module = fragmentShaderModule;
    fragmentShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

    // Describe vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 2;
    vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

    // Describe input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Describe viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = swapChainExtent.width;
    scissor.extent.height = swapChainExtent.height;

    // Note: scissor test is always enabled (although dynamic scissor is possible)
    // Number of viewports must match number of scissors
    VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
    viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = &viewport;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = &scissor;

    // Describe rasterization
    // Note: depth bias and using polygon modes other than fill require changes to logical device creation (device features)
    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
    rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationCreateInfo.depthBiasClamp = 0.0f;
    rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationCreateInfo.lineWidth = 1.0f;

    // Describe multisampling
    // Note: using multisampling also requires turning on device features
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleCreateInfo.minSampleShading = 1.0f;
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

    // Describing color blending
    // Note: all paramaters except blendEnable and colorWriteMask are irrelevant here
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // Note: all attachments must have the same values unless a device feature is enabled
    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;
    colorBlendCreateInfo.blendConstants[0] = 0.0f;
    colorBlendCreateInfo.blendConstants[1] = 0.0f;
    colorBlendCreateInfo.blendConstants[2] = 0.0f;
    colorBlendCreateInfo.blendConstants[3] = 0.0f;

    // Describe pipeline layout
    // Note: this describes the mapping between memory and shader resources (descriptor sets)
    // This is for uniform buffers and samplers
    auto buffers = Buffer::GetAllBuffers();
    std::vector<VkDescriptorSetLayoutBinding> bindings(buffers.size());
    for (size_t i = 0; i < buffers.size(); i++) {
        bindings[i] = {};
        bindings[i].binding = buffers[i]->GetBindingPoint();
        bindings[i].descriptorType = buffers[i]->GetDescriptorType();
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
    descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutCreateInfo.bindingCount = buffers.size();
    descriptorLayoutCreateInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &descriptorLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        RT_ERROR("failed to create descriptor layout");
        exit(1);
    }
    else {
        RT_INFO("created descriptor layout");
    }

    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;
    layoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        RT_ERROR("failed to create pipeline layout");
        exit(1);
    }
    else {
        RT_INFO("created pipeline layout");
    }

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        RT_ERROR("failed to create graphics pipeline");
        exit(1);
    }
    else {
        RT_INFO("created graphics pipeline");
    }

    // No longer necessary
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
}

void GraphicsPipeline::CreateCommandBuffers() {
    // Allocate graphics command buffers
    graphicsCommandBuffers.resize(swapChainImages.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)swapChainImages.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, graphicsCommandBuffers.data()) != VK_SUCCESS) {
        RT_ERROR("failed to allocate graphics command buffers");
        exit(1);
    }
    else {
        RT_INFO("allocated graphics command buffers");
    }

    // Prepare data for recording command buffers
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkImageSubresourceRange subResourceRange = {};
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.levelCount = 1;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.layerCount = 1;

    VkClearValue clearColor = {
        { 0.1f, 0.1f, 0.1f, 1.0f } // R, G, B, A
    };

    // Record command buffer for each swap image
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkBeginCommandBuffer(graphicsCommandBuffers[i], &beginInfo);

        // If present queue family and graphics queue family are different, then a barrier is necessary
        // The barrier is also needed initially to transition the image to the present layout
        VkImageMemoryBarrier presentToDrawBarrier = {};
        presentToDrawBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presentToDrawBarrier.srcAccessMask = 0;
        presentToDrawBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        presentToDrawBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        presentToDrawBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        if (presentQueueFamily != graphicsQueueFamily) {
            presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        }
        else {
            presentToDrawBarrier.srcQueueFamilyIndex = presentQueueFamily;
            presentToDrawBarrier.dstQueueFamilyIndex = graphicsQueueFamily;
        }

        presentToDrawBarrier.image = swapChainImages[i];
        presentToDrawBarrier.subresourceRange = subResourceRange;

        vkCmdPipelineBarrier(graphicsCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentToDrawBarrier);

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = swapChainFramebuffers[i];
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent = swapChainExtent;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(graphicsCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindDescriptorSets(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        vkCmdBindPipeline(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(graphicsCommandBuffers[i], 0, 1, &vertexBuffer, &offset);

        vkCmdBindIndexBuffer(graphicsCommandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(graphicsCommandBuffers[i], 6, 1, 0, 0, 0);

        vkCmdEndRenderPass(graphicsCommandBuffers[i]);

        // If present and graphics queue families differ, then another barrier is required
        if (presentQueueFamily != graphicsQueueFamily) {
            VkImageMemoryBarrier drawToPresentBarrier = {};
            drawToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            drawToPresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            drawToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            drawToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            drawToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            drawToPresentBarrier.srcQueueFamilyIndex = graphicsQueueFamily;
            drawToPresentBarrier.dstQueueFamilyIndex = presentQueueFamily;
            drawToPresentBarrier.image = swapChainImages[i];
            drawToPresentBarrier.subresourceRange = subResourceRange;

            vkCmdPipelineBarrier(graphicsCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &drawToPresentBarrier);
        }

        if (vkEndCommandBuffer(graphicsCommandBuffers[i]) != VK_SUCCESS) {
            RT_ERROR("failed to record command buffer");
            exit(1);
        }
    }

    RT_INFO("recorded command buffers");

    // No longer needed
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void GraphicsPipeline::CreateImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    // Create an image view for every image in the swap chain
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            RT_ERROR("failed to create image view for swap chain image #{0}", i);
            exit(1);
        }
    }

    RT_INFO("created image views for swap chain images");
}

void GraphicsPipeline::CreateFramebuffers() {
    swapChainFramebuffers.resize(swapChainImages.size());

    // Note: Framebuffer is basically a specific choice of attachments for a render pass
    // That means all attachments must have the same dimensions, interesting restriction
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &swapChainImageViews[i];
        createInfo.width = swapChainExtent.width;
        createInfo.height = swapChainExtent.height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            RT_ERROR("failed to create framebuffer for swap chain image view #{0}", i);
            exit(1);
        }
    }

    RT_INFO("created framebuffers for swap chain image views");
}