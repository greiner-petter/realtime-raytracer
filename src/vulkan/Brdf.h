#ifndef BRDF_H
#define BRDF_H

#include <string>
#include <vector>
#include "common/Types.h"

using BrdfID = int32_t;

static constexpr BrdfID NULL_BRDF = -1;

class Brdf {
public:
    Brdf(const std::string& filepath);
    ~Brdf();

    BrdfID GetId() const { return id; }
    const float* GetData() const { return data.data(); }
    size_t GetDataSize() const { return data.size(); }

    // Get the data offset for a BRDF ID (resolves ID to GPU buffer offset)
    static uint32_t GetDataOffset(BrdfID brdfId);

    static void CreateGPUBuffers();

private:
    static void UploadToGPU();

    std::vector<float> data;
    BrdfID id;
};

#endif
