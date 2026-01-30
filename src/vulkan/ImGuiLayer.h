#ifndef IMGUI_LAYER_H
#define IMGUI_LAYER_H

#include <vulkan/vulkan.h>
#include <vector>

class ImGuiLayer {
public:
    static void Init();
    static void Cleanup();
    static void OnWindowResize();

    static void BeginFrame();
    static void EndFrame();
    static void RecordCommands(VkCommandBuffer cmd, uint32_t imageIndex);

private:
    static void CreateRenderPass();
    static void CreateFramebuffers();
    static void DestroyFramebuffers();

    inline static VkRenderPass renderPass = VK_NULL_HANDLE;
    inline static VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    inline static std::vector<VkFramebuffer> framebuffers;
};

#endif
