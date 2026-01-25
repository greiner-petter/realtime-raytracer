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
// 3 channels (RGB) = 4,374,000 doubles in total
#define BRDF_TOTAL_SAMPLES (BRDF_SAMPLING_RES_THETA_H * BRDF_SAMPLING_RES_THETA_D * BRDF_SAMPLING_RES_PHI_D / 2)

struct BrdfShader : public TypedShader<ShaderType::BrdfShader> {
    BrdfShader(const char* brdfFilePath, Vec3 const& colorScale = Vec3(1.0f))
        : scale(Vec4(colorScale, 0.0f)) {
        LoadFromFile(brdfFilePath);
    }

    ~BrdfShader() {
        if (rawData) {
            free(rawData);
            rawData = nullptr;
        }
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

        rawData = (double*)malloc(sizeof(double) * 3 * n);
        numbytes = fread(rawData, sizeof(double), 3 * n, f);
        if (numbytes != static_cast<size_t>(3 * n)) {
            fprintf(stderr, "Failed to read BRDF data\n");
            free(rawData);
            rawData = nullptr;
            fclose(f);
            return false;
        }

        fclose(f);
        rawDataSize = 3 * n;
        return true;
    }

    // Convert double data to float for GPU upload
    std::vector<float> GetFloatData() const {
        if (!rawData) return {};

        std::vector<float> floatData(rawDataSize);
        for (size_t i = 0; i < rawDataSize; ++i) {
            floatData[i] = static_cast<float>(rawData[i]);
        }
        return floatData;
    }

    bool IsLoaded() const { return rawData != nullptr; }
    size_t GetFloatDataSize() const { return rawDataSize; }

    // For the shader metadata - stores offset and scale
    virtual void* GetDataLayoutBeginPtr() override { return &dataOffset; }
    virtual size_t GetDataSize() const override { return sizeof(dataOffset) + sizeof(scale); }

    Vec4 dataOffset = Vec4(0.0f);  // x = offset into brdfData buffer (in floats)
    Vec4 scale = Vec4(1.0f);       // xyz = color scale
    double* rawData = nullptr;
    size_t rawDataSize = 0;
};

#endif
