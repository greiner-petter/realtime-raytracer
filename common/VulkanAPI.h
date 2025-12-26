#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct UBO {
    glm::vec2 u_resolution;
    float u_aspectRatio;
    float u_FocusDistance = 1.0f;
    glm::vec4 u_CameraPosition = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 u_CameraForward = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    glm::vec4 u_CameraRight = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 u_CameraUp = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
};

class VulkanAPI {
public:
    static void SetupVulkan();
    
    static void CreateInstance();
    static void CreateDebugCallback();
    static void CreateWindowSurface();
    static void FindPhysicalDevice();
    static void CheckSwapChainSupport();
    static void FindQueueFamilies();
    static void CreateLogicalDevice();
    static void CreateSemaphores();
    static void CreateCommandPool();
    static void CreateVertexBuffer();
    static void CreateUniformBuffer();
    static void CreateSceneStorageBuffer();
    static void CreateSwapChain();
    static void CreateRenderPass();
    static void CreateImageViews();
    static void CreateFramebuffers();
    static void CreateGraphicsPipeline();
    static void CreateDescriptorPool();
    static void CreateDescriptorSet();
    static void CreateCommandBuffers();

    static void UpdateUniformData();
    static void UpdateSceneData();

    static void CleanUp(bool fullClean);

    static void OnWindowSizeChanged();
    static void Draw();
};