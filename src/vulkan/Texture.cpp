#include "Texture.h"
#include "Buffer.h"
#include "common/Log.h"
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <third-party/stb_image.h>

static std::vector<Texture*> s_AllTextures;
static std::shared_ptr<SSBO> s_TextureSSBO;
static std::shared_ptr<SSBO> s_DataSSBO;
#define SSBO_TEXTURE_DATA_SIZE (512 * 1024 * 1024) /* 512 Mb */

void Texture::CreateGPUBuffers() {
    s_TextureSSBO = SSBO::Create(51);
    s_DataSSBO = SSBO::Create(52, SSBO_TEXTURE_DATA_SIZE);
}

Texture::Texture(const std::string& filepath) {
    id = s_AllTextures.size();
    s_AllTextures.push_back(this);

    int32_t c;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &width, &height, &c, STBI_rgb_alpha);
    if (!pixels) throw std::runtime_error("Failed to load texture: " + filepath);

    data.reserve(width * height);
    for (int i = 0; i < width * height; ++i) {
        int idx = i * 4;
        float r = pixels[idx] / 255.0f;
        float g = pixels[idx + 1] / 255.0f;
        float b = pixels[idx + 2] / 255.0f;
        float a = pixels[idx + 3] / 255.0f;
        data.emplace_back(r, g, b, a);
    }

    stbi_image_free(pixels);
    RT_INFO("Loaded Texture. Width: {} Height: {} Data Total: {} bytes", width, height, sizeof(Vec4) * data.size());
    Texture::UploadToGPU();
}

Texture::~Texture() {
    s_AllTextures[GetId()] = nullptr;
    data.clear();
    Texture::UploadToGPU();
}

void Texture::UploadToGPU() {
    if (VulkanContext::GetDevice() == VK_NULL_HANDLE) {
        return;
    }

    int32_t dataCount = 0;
    size_t GPUDataSize = sizeof(uint32_t) + sizeof(uint32_t) * 3 * s_AllTextures.size();
    void* DataGPU = s_TextureSSBO->MapData(GPUDataSize);

    const uint32_t count = static_cast<uint32_t>(s_AllTextures.size());
    std::memcpy(DataGPU, &count, sizeof(uint32_t));

    for (uint32_t i = 0; i < s_AllTextures.size(); i++) {
        if (!s_AllTextures[i]) {
            continue;
        }
        std::memcpy(static_cast<byte*>(DataGPU) + sizeof(uint32_t) + i * sizeof(uint32_t) * 3 + sizeof(uint32_t) * 0, &dataCount, sizeof(uint32_t));
        std::memcpy(static_cast<byte*>(DataGPU) + sizeof(uint32_t) + i * sizeof(uint32_t) * 3 + sizeof(uint32_t) * 1, &s_AllTextures[i]->width, sizeof(uint32_t));
        std::memcpy(static_cast<byte*>(DataGPU) + sizeof(uint32_t) + i * sizeof(uint32_t) * 3 + sizeof(uint32_t) * 2, &s_AllTextures[i]->height, sizeof(uint32_t));
        dataCount += s_AllTextures[i]->data.size();
    }
    s_TextureSSBO->UnmapData();

    // Upload Data
    DataGPU = s_DataSSBO->MapData(sizeof(Vec4) * dataCount);
    int32_t dataWritten = 0;
    for (uint32_t i = 0; i < s_AllTextures.size(); i++) {
        if (!s_AllTextures[i]) {
            continue;
        }
        std::memcpy(static_cast<byte*>(DataGPU) + sizeof(Vec4) * dataWritten, &s_AllTextures[i]->data[0], sizeof(Vec4) * s_AllTextures[i]->data.size());
        dataWritten += s_AllTextures[i]->data.size();
    }
    if (dataWritten > 0) {
        s_DataSSBO->UnmapData();
    }
}