#include "ImGuiLayer.h"
#include "VulkanContext.h"
#include "Swapchain.h"
#include "common/Window.h"
#include "common/Log.h"

#include "third-party/imgui/imgui.h"
#include "third-party/imgui/backends/imgui_impl_glfw.h"
#include "third-party/imgui/backends/imgui_impl_vulkan.h"

#include <GLFW/glfw3.h>

void ImGuiLayer::Init() {
    // Create descriptor pool for ImGui
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
    };

    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 100;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(VulkanContext::GetDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        RT_ERROR("Failed to create ImGui descriptor pool");
        return;
    }

    CreateRenderPass();
    CreateFramebuffers();

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup style
    ImGui::StyleColorsDark();

    // Initialize platform/renderer backends
    ImGui_ImplGlfw_InitForVulkan(Window::GetGLFWwindow(), true);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.ApiVersion = VK_API_VERSION_1_3;
    initInfo.Instance = VulkanContext::GetInstance();
    initInfo.PhysicalDevice = VulkanContext::GetPhysicalDevice();
    initInfo.Device = VulkanContext::GetDevice();
    initInfo.QueueFamily = VulkanContext::GetQueueFamilyIndex();
    initInfo.Queue = VulkanContext::GetGraphicsQueue();
    initInfo.DescriptorPool = descriptorPool;
    initInfo.MinImageCount = Swapchain::GetImageCount();
    initInfo.ImageCount = Swapchain::GetImageCount();
    initInfo.PipelineInfoMain.RenderPass = renderPass;
    initInfo.PipelineInfoMain.Subpass = 0;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo);
}

void ImGuiLayer::Cleanup() {
    VulkanContext::DeviceWaitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    DestroyFramebuffers();

    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(VulkanContext::GetDevice(), renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(VulkanContext::GetDevice(), descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
}

void ImGuiLayer::OnWindowResize() {
    DestroyFramebuffers();
    CreateFramebuffers();
    ImGui_ImplVulkan_SetMinImageCount(Swapchain::GetImageCount());
}

void ImGuiLayer::BeginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::EndFrame() {
    ImGui::Render();
}

void ImGuiLayer::RecordCommands(VkCommandBuffer cmd, uint32_t imageIndex) {
    VkRenderPassBeginInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = Swapchain::GetExtent();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRenderPass(cmd);
}

void ImGuiLayer::CreateRenderPass() {
    VkAttachmentDescription attachment = {};
    attachment.format = Swapchain::GetFormat();
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;  // Preserve existing content (raytraced image)
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef = {};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &attachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(VulkanContext::GetDevice(), &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS) {
        RT_ERROR("Failed to create ImGui render pass");
    }
}

void ImGuiLayer::CreateFramebuffers() {
    const auto& imageViews = Swapchain::GetImageViews();
    framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &imageViews[i];
        framebufferInfo.width = Swapchain::GetExtent().width;
        framebufferInfo.height = Swapchain::GetExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(VulkanContext::GetDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            RT_ERROR("Failed to create ImGui framebuffer");
        }
    }
}

void ImGuiLayer::DestroyFramebuffers() {
    for (auto fb : framebuffers) {
        if (fb != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(VulkanContext::GetDevice(), fb, nullptr);
        }
    }
    framebuffers.clear();
}
