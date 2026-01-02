#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class GraphicsPipeline {
public:
    static std::shared_ptr<GraphicsPipeline> Create();
    void Refresh();
    void PartialCleanUp();

    void CreateSwapChain();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateCommandBuffers();
    void CreateImageViews();
    void CreateFramebuffers();
    
public:
    VkExtent2D swapChainExtent = {1280, 720};
    VkFormat swapChainFormat;
    VkSwapchainKHR oldSwapChain;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
    std::vector<VkCommandBuffer> graphicsCommandBuffers;
};

#endif