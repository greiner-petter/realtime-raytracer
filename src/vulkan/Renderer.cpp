#include "Renderer.h"
#include "VulkanContext.h"
#include "Swapchain.h"
#include "OffscreenResources.h"
#include "ComputePipeline.h"
#include "Buffer.h"
#include "Texture.h"
#include "scene/Scene.h"
#include "common/Log.h"
#include "common/Params.h"

void Renderer::Init() {
    VulkanContext::Init();

    if (Params::IsInteractiveMode()) {
        Swapchain::Init();
    }

    OffscreenResources::Init();
    Scene::CreateGPUBuffers();
    Texture::CreateGPUBuffers();
    ComputePipeline::Init();

    if (Params::IsInteractiveMode()) {
        CreateSyncObjects();
        CreateCommandBuffers();
        RecordCommandBuffers();
    } else {
        CreateHeadlessCommandBuffer();
        RecordHeadlessCommandBuffer();
    }
}

void Renderer::Cleanup() {
    VulkanContext::DeviceWaitIdle();

    Buffer::DestroyAllBuffers();

    if (Params::IsInteractiveMode()) {
        vkDestroySemaphore(VulkanContext::GetDevice(), imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(VulkanContext::GetDevice(), renderingFinishedSemaphore, nullptr);
        vkDestroyFence(VulkanContext::GetDevice(), inFlightFence, nullptr);
        FreeCommandBuffers();
        Swapchain::Cleanup();
    } else {
        vkFreeCommandBuffers(VulkanContext::GetDevice(), VulkanContext::GetCommandPool(), 1, &headlessCommandBuffer);
    }

    ComputePipeline::Cleanup();
    OffscreenResources::Cleanup();
    VulkanContext::Cleanup();
}

void Renderer::OnWindowSizeChanged() {
    RT_ASSERT(Params::IsInteractiveMode(), "OnWindowSizeChanged can only be called in interactive mode");
    VulkanContext::DeviceWaitIdle();

    Swapchain::Recreate();
    OffscreenResources::Resize();
    ComputePipeline::UpdateDescriptorSets();
    
    FreeCommandBuffers();
    CreateCommandBuffers();
    RecordCommandBuffers();
}

void Renderer::Draw() {
    if (!Params::IsInteractiveMode()) {
        DrawHeadless();
        return;
    }

    vkWaitForFences(VulkanContext::GetDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(VulkanContext::GetDevice(), Swapchain::GetHandle(), UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        OnWindowSizeChanged();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        RT_ERROR("failed to acquire swap chain image");
        return;
    }

    vkResetFences(VulkanContext::GetDevice(), 1, &inFlightFence);

    VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    VkSemaphore waitSems[] = { imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = waitSems;
    submit.pWaitDstStageMask = waitStages;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffers[imageIndex];
    VkSemaphore sigSems[] = { renderingFinishedSemaphore };
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = sigSems;

    vkQueueSubmit(VulkanContext::GetGraphicsQueue(), 1, &submit, inFlightFence);

    VkPresentInfoKHR present = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = sigSems;
    VkSwapchainKHR swapChains[] = { Swapchain::GetHandle() };
    present.swapchainCount = 1;
    present.pSwapchains = swapChains;
    present.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(VulkanContext::GetGraphicsQueue(), &present);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        OnWindowSizeChanged();
    }
}

void Renderer::DrawHeadless() {
    VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &headlessCommandBuffer;

    vkQueueSubmit(VulkanContext::GetGraphicsQueue(), 1, &submit, VK_NULL_HANDLE);

    vkQueueWaitIdle(VulkanContext::GetGraphicsQueue());
}

void Renderer::CreateSyncObjects() {
    VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(VulkanContext::GetDevice(), &semInfo, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(VulkanContext::GetDevice(), &semInfo, nullptr, &renderingFinishedSemaphore);
    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
    vkCreateFence(VulkanContext::GetDevice(), &fenceInfo, nullptr, &inFlightFence);
}

void Renderer::CreateCommandBuffers() {
    commandBuffers.resize(Swapchain::GetImages().size());
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = VulkanContext::GetCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
    vkAllocateCommandBuffers(VulkanContext::GetDevice(), &allocInfo, commandBuffers.data());
}

void Renderer::FreeCommandBuffers() {
    vkFreeCommandBuffers(VulkanContext::GetDevice(), VulkanContext::GetCommandPool(), (uint32_t)commandBuffers.size(), commandBuffers.data());
    commandBuffers.clear();
}

void Renderer::CreateHeadlessCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = VulkanContext::GetCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(VulkanContext::GetDevice(), &allocInfo, &headlessCommandBuffer);
}

void Renderer::RecordCommandBuffers() {
    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBuffer cmd = commandBuffers[i];
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.image = OffscreenResources::GetImage();
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::GetPipeline());
        VkDescriptorSet ds = ComputePipeline::GetDescriptorSet();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline::GetLayout(), 0, 1, &ds, 0, nullptr);
        
        vkCmdDispatch(cmd, (Swapchain::GetExtent().width + 15) / 16, (Swapchain::GetExtent().height + 15) / 16, 1);

        VkImageMemoryBarrier barriers[2] = {};
        barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barriers[0].image = OffscreenResources::GetImage();
        barriers[0].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[1].srcAccessMask = 0;
        barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[1].image = Swapchain::GetImages()[i];
        barriers[1].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);

        VkImageBlit blit = {};
        blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        blit.srcOffsets[1] = { (int32_t)Swapchain::GetExtent().width, (int32_t)Swapchain::GetExtent().height, 1 };
        blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        blit.dstOffsets[1] = { (int32_t)Swapchain::GetExtent().width, (int32_t)Swapchain::GetExtent().height, 1 };

        vkCmdBlitImage(cmd, OffscreenResources::GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Swapchain::GetImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);

        VkImageMemoryBarrier presentBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        presentBarrier.dstAccessMask = 0;
        presentBarrier.image = Swapchain::GetImages()[i];
        presentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentBarrier);

        vkEndCommandBuffer(cmd);
    }
}

void Renderer::RecordHeadlessCommandBuffer() {
    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    vkBeginCommandBuffer(headlessCommandBuffer, &beginInfo);

    VkImageMemoryBarrier toGeneral = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    toGeneral.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toGeneral.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    toGeneral.srcAccessMask = 0;
    toGeneral.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    toGeneral.image = OffscreenResources::GetImage();
    toGeneral.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(
        headlessCommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &toGeneral
    );

    vkCmdBindPipeline(
        headlessCommandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        ComputePipeline::GetPipeline()
    );

    VkDescriptorSet ds = ComputePipeline::GetDescriptorSet();
    vkCmdBindDescriptorSets(
        headlessCommandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        ComputePipeline::GetLayout(),
        0, 1, &ds, 0, nullptr
    );

    vkCmdDispatch(
        headlessCommandBuffer,
        (OffscreenResources::GetWidth() + 15) / 16,
        (OffscreenResources::GetHeight() + 15) / 16,
        1
    );

    VkImageMemoryBarrier toTransferSrc = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    toTransferSrc.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    toTransferSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toTransferSrc.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    toTransferSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    toTransferSrc.image = OffscreenResources::GetImage();
    toTransferSrc.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(
        headlessCommandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &toTransferSrc
    );

    vkEndCommandBuffer(headlessCommandBuffer);
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third-party/stb_image_write.h"

void Renderer::SaveCurrentFrameToDisk(const std::string& filePath) {
    auto device = VulkanContext::GetDevice();
    auto width = OffscreenResources::GetWidth();
    auto height = OffscreenResources::GetHeight();
    auto image = OffscreenResources::GetImage();
    VkDeviceSize size = VkDeviceSize(width) * height * sizeof(float) * 4;

    VkBuffer buffer;
    VkDeviceMemory memory;
    Buffer::Create(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, memory);

    VkCommandBuffer cmd = VulkanContext::BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.image = image;
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VulkanContext::EndSingleTimeCommands(cmd);

    float* srcPixels = nullptr;
    vkMapMemory(device, memory, 0, size, 0, reinterpret_cast<void**>(&srcPixels));

    std::vector<uint8_t> ldr(width * height * 4);
    for (uint32_t i = 0; i < width * height; ++i) {
        ldr[4*i+0] = uint8_t(std::clamp(srcPixels[4*i+0], 0.0f, 1.0f) * 255.0f); 
        ldr[4*i+1] = uint8_t(std::clamp(srcPixels[4*i+1], 0.0f, 1.0f) * 255.0f); 
        ldr[4*i+2] = uint8_t(std::clamp(srcPixels[4*i+2], 0.0f, 1.0f) * 255.0f); 
        ldr[4*i+3] = 255; 
    }

    vkUnmapMemory(device, memory);
    stbi_write_png(filePath.c_str(), width, height, 4, ldr.data(), width * 4);
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
}
