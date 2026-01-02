#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class GraphicsPipeline {
public:
    static std::shared_ptr<GraphicsPipeline> Create();
    void Refresh();
    void ClearPipeline();
    void ClearSwapChain();
    void ClearCommandBuffers();

    void CreateSwapChain();
    void CreateGraphicsPipeline();
    void CreateCommandBuffers();
    void CreateImageViews();
    
public:
    VkExtent2D swapChainExtent = {1280, 720};
    VkFormat swapChainFormat;
    VkSwapchainKHR oldSwapChain;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> graphicsCommandBuffers;
};

#endif