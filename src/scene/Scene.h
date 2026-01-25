#ifndef SCENE_H
#define SCENE_H

#include "common/Types.h"
#include "primitives/Primitive.h"
#include "shaders/Shader.h"
#include "lights/Light.h"
#include "vulkan/Buffer.h"
#include "scene/KDTree.h"
#include <cstring>

struct alignas(16) UBO {
    glm::vec2 u_resolution;
    float u_aspectRatio;
    float u_FocusDistance = 1.0f;
    uint32_t u_SampleIndex = 0;
    float u_Seed = 0.0f;
    uint32_t u_Raybounces = 4;
    uint32_t u_EnableGI = 0;
    glm::vec4 u_CameraPosition = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 u_CameraForward = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    glm::vec4 u_CameraRight = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 u_CameraUp = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
};

class Scene {
public:
    Scene() = default;
    virtual ~Scene() = default;
    static void CreateGPUBuffers();
    void BuildKDTree();

    template <typename T, typename EnumType>
    void WriteBufferForType(const std::vector<std::shared_ptr<T>>& collection, EnumType typeToFind, SSBO& ssbo) {
        size_t typeCount = 0;
        size_t sizeOfType = 0;
        uint32_t indiceIt = 0;
        std::vector<std::shared_ptr<T>> itemsOfType;
        for (const auto& item : collection) {
            if (item->type == typeToFind) {
                item->index = indiceIt++;
                itemsOfType.push_back(item);
                typeCount++;
                sizeOfType = item->GetDataSize();
            }
        }

        if (typeCount == 0) {
            return;
        }

        size_t GPUDataSize = sizeof(uint32_t)        // count
                        + sizeof(uint32_t) * 3       // padding (align 16)
                        + sizeOfType * typeCount;    // data
        
        void* DataGPU = ssbo.MapData(GPUDataSize);
        
        // Copy count
        const uint32_t count = static_cast<uint32_t>(typeCount);
        std::memcpy(DataGPU, &count, sizeof(uint32_t));

        // Copy data
        for (auto& item : itemsOfType) {
            std::memcpy(static_cast<byte*>(DataGPU) + sizeof(uint32_t) * 4 + item->index * sizeOfType, 
                        item->GetDataLayoutBeginPtr(), 
                        sizeOfType);
        }
        ssbo.UnmapData();
    }

    void UploadMeshTrianglesToGPU();
    void ConvertSceneToGPUData();
    void UpdateGPUBuffers();
    bool IsBufferDirty() const { return m_IsBufferDirty; }
    void SetBufferDirty(bool dirty) { m_IsBufferDirty = dirty; }

    void AddPrimitive(const std::shared_ptr<Primitive>& primitive) {
        m_Primitives.push_back(primitive);
        m_IsBufferDirty = true;
    }

    void AddShader(const std::shared_ptr<Shader>& shader) {
        m_Shaders.push_back(shader);
        m_IsBufferDirty = true;
    }

    void AddLight(const std::shared_ptr<Light>& light) {
        m_Lights.push_back(light);
        m_IsBufferDirty = true;
    }

    void ClearScene();

    void UploadKDTreeToGPU();

private:
    inline static std::shared_ptr<UniformBuffer> uniformBuffer;
    inline static std::shared_ptr<SSBO> kdTreeSSBO;        // KD-tree nodes
    inline static std::shared_ptr<SSBO> kdTreeIndicesSSBO; // Primitive indices for leaves
    inline static std::shared_ptr<SSBO> meshTrianglesSSBO;

    inline static std::shared_ptr<SSBO> primitiveSSBO;
    inline static std::shared_ptr<SSBO> sphereSSBO;
    inline static std::shared_ptr<SSBO> triangleSSBO;
    inline static std::shared_ptr<SSBO> planeSSBO;
    inline static std::shared_ptr<SSBO> boxSSBO;
    inline static std::shared_ptr<SSBO> meshSSBO;

    inline static std::shared_ptr<SSBO> shaderSSBO;
    inline static std::shared_ptr<SSBO> flatSSBO;
    inline static std::shared_ptr<SSBO> refractionSSBO;
    inline static std::shared_ptr<SSBO> mirrorSSBO;
    inline static std::shared_ptr<SSBO> simpleShadowSSBO;
    inline static std::shared_ptr<SSBO> lambertSSBO;
    inline static std::shared_ptr<SSBO> phongSSBO;
    inline static std::shared_ptr<SSBO> cookTorranceSSBO;
    inline static std::shared_ptr<SSBO> simpleTextureSSBO;
    inline static std::shared_ptr<SSBO> brdfShaderSSBO;
    inline static std::shared_ptr<SSBO> brdfDataSSBO;

    inline static std::shared_ptr<SSBO> lightSSBO;
    inline static std::shared_ptr<SSBO> pointSSBO;
    inline static std::shared_ptr<SSBO> ambientSSBO;
    inline static std::shared_ptr<SSBO> spotSSBO;

    std::vector<std::shared_ptr<Primitive>> m_Primitives;
    std::vector<std::shared_ptr<Shader>> m_Shaders;
    std::vector<std::shared_ptr<Light>> m_Lights;

    KDTree m_KDTree;

    bool m_IsBufferDirty = true;
};

#endif
