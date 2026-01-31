#include "Brdf.h"
#include "Buffer.h"
#include "VulkanContext.h"
#include "common/Log.h"
#include <stdexcept>
#include <cstdio>

#define BRDF_SAMPLING_RES_THETA_H 90
#define BRDF_SAMPLING_RES_THETA_D 90
#define BRDF_SAMPLING_RES_PHI_D 360

#define BRDF_TOTAL_SAMPLES (BRDF_SAMPLING_RES_THETA_H * BRDF_SAMPLING_RES_THETA_D * BRDF_SAMPLING_RES_PHI_D / 2)
#define BRDF_DATA_SIZE (BRDF_TOTAL_SAMPLES * 3)
#define SSBO_BRDF_DATA_SIZE (256 * 1024 * 1024)  // 256 MB - enough for ~14 BRDFs

static std::vector<Brdf*> s_AllBrdfs;
static std::shared_ptr<SSBO> s_BrdfDataSSBO;  // Binding 4: BRDF sample data

void Brdf::CreateGPUBuffers() {
    s_BrdfDataSSBO = SSBO::Create(53, SSBO_BRDF_DATA_SIZE);
}

uint32_t Brdf::GetDataOffset(BrdfID brdfId) {
    if (brdfId < 0 || brdfId >= static_cast<BrdfID>(s_AllBrdfs.size())) {
        return 0;
    }
    // All BRDFs have the same size, so offset = id * size
    return static_cast<uint32_t>(brdfId) * BRDF_DATA_SIZE;
}

Brdf::Brdf(const std::string& filepath) {
    id = s_AllBrdfs.size();
    s_AllBrdfs.push_back(this);

    FILE* f = fopen(filepath.c_str(), "rb");
    if (!f) {
        throw std::runtime_error("Cannot open BRDF file: " + filepath);
    }

    int dims[3];
    size_t numbytes = fread(dims, sizeof(int), 3, f);
    if (numbytes != 3) {
        fclose(f);
        throw std::runtime_error("Failed to read BRDF dimensions: " + filepath);
    }

    int n = dims[0] * dims[1] * dims[2];
    if (n != BRDF_TOTAL_SAMPLES) {
        fclose(f);
        throw std::runtime_error("BRDF dimensions don't match: expected " +
            std::to_string(BRDF_TOTAL_SAMPLES) + ", got " + std::to_string(n));
    }

    // Read as double, convert to float to save GPU memory
    std::vector<double> tempData(3 * n);
    size_t itemsRead = fread(tempData.data(), sizeof(double), 3 * n, f);
    fclose(f);

    if (itemsRead != static_cast<size_t>(3 * n)) {
        throw std::runtime_error("Failed to read BRDF data: " + filepath);
    }

    // Convert double to float
    data.resize(3 * n);
    for (size_t i = 0; i < tempData.size(); ++i) {
        data[i] = static_cast<float>(tempData[i]);
    }

    RT_INFO("Loaded BRDF. ID: {} Samples: {} Data Total: {} bytes", id, BRDF_TOTAL_SAMPLES, sizeof(float) * data.size());
    Brdf::UploadToGPU();
}

Brdf::~Brdf() {
    s_AllBrdfs[GetId()] = nullptr;
    data.clear();
    Brdf::UploadToGPU();
}

void Brdf::UploadToGPU() {
    if (VulkanContext::GetDevice() == VK_NULL_HANDLE) {
        return;
    }

    if (s_AllBrdfs.empty()) {
        return;
    }

    // All BRDFs have the same size - allocate for all of them
    size_t totalDataFloats = s_AllBrdfs.size() * BRDF_DATA_SIZE;

    RT_INFO("Uploading {} BRDFs, total {} floats", s_AllBrdfs.size(), totalDataFloats);

    void* dataGPU = s_BrdfDataSSBO->MapData(sizeof(float) * totalDataFloats);
    byte* dst = static_cast<byte*>(dataGPU);

    // Upload each BRDF at its ID * BRDF_DATA_SIZE offset
    for (size_t i = 0; i < s_AllBrdfs.size(); i++) {
        const Brdf* brdf = s_AllBrdfs[i];
        if (brdf && brdf->GetDataSize() == BRDF_DATA_SIZE) {
            std::memcpy(dst, brdf->GetData(), BRDF_DATA_SIZE * sizeof(float));
        }
        // Always advance by BRDF_DATA_SIZE to maintain consistent offsets
        dst += BRDF_DATA_SIZE * sizeof(float);
    }
    s_BrdfDataSSBO->UnmapData();
}
