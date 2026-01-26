#ifndef BRDFSHADER_H
#define BRDFSHADER_H

#include "shaders/Shader.h"
#include "common/Types.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

#define BRDF_SAMPLING_RES_THETA_H 90
#define BRDF_SAMPLING_RES_THETA_D 90
#define BRDF_SAMPLING_RES_PHI_D 360

// Total number of samples: 90 * 90 * 180 = 1,458,000 per channel
// 3 channels (RGB) = 4,374,000 floats in total
#define BRDF_TOTAL_SAMPLES (BRDF_SAMPLING_RES_THETA_H * BRDF_SAMPLING_RES_THETA_D * BRDF_SAMPLING_RES_PHI_D / 2)
#define BRDF_DATA_SIZE (BRDF_TOTAL_SAMPLES * 3)

struct BrdfShader : public TypedShader<ShaderType::BrdfShader> {
    BrdfShader(const char* brdfFilePath, Vec3 const& colorScale = Vec3(1.0f))
        : scaleIndex(Vec4(colorScale, 0.0f)) {
        LoadFromFile(brdfFilePath);
    }

    bool LoadFromFile(const char* filename) {
        FILE* f = fopen(filename, "rb");
        if (!f) {
            fprintf(stderr, "Cannot open BRDF file: %s\n", filename);
            return false;
        }

        int dims[3];
        size_t numbytes = fread(dims, sizeof(int), 3, f);
        if (numbytes != 3) {
            fprintf(stderr, "Failed to read BRDF dimensions\n");
            fclose(f);
            return false;
        }

        int n = dims[0] * dims[1] * dims[2];
        if (n != BRDF_TOTAL_SAMPLES) {
            fprintf(stderr, "BRDF dimensions don't match: expected %d, got %d\n", BRDF_TOTAL_SAMPLES, n);
            fclose(f);
            return false;
        }

        // Read as double, convert to float to save GPU memory
        std::vector<double> tempData(3 * n);
        size_t itemsRead = fread(tempData.data(), sizeof(double), 3 * n, f);
        if (itemsRead != static_cast<size_t>(3 * n)) {
            fprintf(stderr, "Failed to read BRDF data\n");
            fclose(f);
            return false;
        }

        // Convert double to float
        data.resize(3 * n);
        for (size_t i = 0; i < tempData.size(); ++i) {
            data[i] = static_cast<float>(tempData[i]);
        }

        fclose(f);
        return true;
    }

    // Metadata only (scaleIndex) - data uploaded separately
    virtual void* GetDataLayoutBeginPtr() override { return &scaleIndex; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4); }

    Vec4 scaleIndex;           // xyz = color scale, w = offset into data buffer
    std::vector<float> data;   // BRDF data (converted from double)
};

#endif
