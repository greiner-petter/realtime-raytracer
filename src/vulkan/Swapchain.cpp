#include "Swapchain.h"
#include "VulkanContext.h"
#include "common/Window.h"
#include "common/Log.h"
#include "common/Params.h"
#include <algorithm>

void Swapchain::Init() {
    RT_ASSERT(Params::IsInteractiveMode(), "Swapchain can only be initialized in interactive mode");
    Create();
}

void Swapchain::Cleanup() {
    RT_ASSERT(Params::IsInteractiveMode(), "Swapchain can only be destroyed in interactive mode");
    if (swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(VulkanContext::GetDevice(), swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
    }
}

void Swapchain::Recreate() {
    Cleanup();
    Create();
}

void Swapchain::Create() {
    RT_ASSERT(Params::IsInteractiveMode(), "Swapchain can only be created in interactive mode");
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanContext::GetPhysicalDevice(), VulkanContext::GetSurface(), &capabilities);

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
    createInfo.surface = VulkanContext::GetSurface();
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

    if (vkCreateSwapchainKHR(VulkanContext::GetDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        RT_ERROR("failed to create swap chain"); exit(1);
    }

    uint32_t count;
    vkGetSwapchainImagesKHR(VulkanContext::GetDevice(), swapChain, &count, nullptr);
    swapChainImages.resize(count);
    vkGetSwapchainImagesKHR(VulkanContext::GetDevice(), swapChain, &count, swapChainImages.data());
}
