#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <vector>
#include "common/Types.h"

using TextureID = int32_t;

static constexpr TextureID NULL_TEXTURE = -1;

class Texture {
public:
    Texture(const std::string& filepath);
    ~Texture();

    TextureID GetId() const { return id; }

    static void CreateGPUBuffers();

private:
    static void UploadToGPU();

    std::vector<Vec4> data;
    int32_t width;
    int32_t height;
    TextureID id;
};


#endif