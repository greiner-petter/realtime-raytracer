#ifndef SCENE_H
#define SCENE_H

#include "common/Types.h"
#include "primitives/Primitive.h"
#include "shaders/Shader.h"
#include "vulkan/Buffer.h"

struct alignas(16) UBO {
    glm::vec2 u_resolution;
    float u_aspectRatio;
    float u_FocusDistance = 1.0f;
    uint32_t u_SampleIndex = 0;
    float u_Seed = 0.0f;
    uint32_t _padding[2];
    glm::vec4 u_CameraPosition = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 u_CameraForward = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    glm::vec4 u_CameraRight = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 u_CameraUp = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
};

struct Node {
    Node() : dimension(0), split(0), startIndex(0), primitiveCount(0) {}

    inline bool isLeaf() const { return child[0] == nullptr && child[1] == nullptr; }

    // Branch split
    std::unique_ptr<Node> child[2];
    int dimension;
    float split;

    // Leaf primitives
    uint32_t startIndex;
    uint32_t primitiveCount;
};

struct GPUKDNode {
    int left;
    int right;
    int axis;
    float split;
    int firstPrim;
    int primCount;
};

class Scene {
public:
    Scene() = default;
    virtual ~Scene() = default;
    static void CreateGPUBuffers();
    void BuildTree(int maximumDepth = 10, int minimumNumberOfPrimitives = 2);

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

    void ClearScene();

    std::unique_ptr<Node> Build(const Vec3& minimumBounds, const Vec3& maximumBounds, int start, int end /* [start, end) */, int depth);
    std::vector<GPUKDNode> FlattenKDTree() const;
    void UploadTreeToGPU();

private:
    inline static std::shared_ptr<UniformBuffer> uniformBuffer;
    inline static std::shared_ptr<SSBO> kdTreeSSBO;

    inline static std::shared_ptr<SSBO> primitiveSSBO;
    inline static std::shared_ptr<SSBO> sphereSSBO;
    inline static std::shared_ptr<SSBO> triangleSSBO;
    inline static std::shared_ptr<SSBO> planeSSBO;
    inline static std::shared_ptr<SSBO> boxSSBO;

    inline static std::shared_ptr<SSBO> shaderSSBO;
    inline static std::shared_ptr<SSBO> flatSSBO;
    inline static std::shared_ptr<SSBO> mirrorSSBO;

    std::vector<std::shared_ptr<Primitive>> m_Primitives;
    std::vector<std::shared_ptr<Shader>> m_Shaders;

    std::unique_ptr<Node> root;
    int maximumDepth = 10;
    int minimumNumberOfPrimitives = 10;
    Vec4 absoluteMinimum, absoluteMaximum; // xyz + padding

    bool m_IsBufferDirty = true;
};

#endif
