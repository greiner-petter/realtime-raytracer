#include "scene/Scene.h"
#include "common/Log.h"
#include "common/Window.h"
#include "common/Params.h"
#include "primitives/Mesh.h"
#include "primitives/Triangle.h"
#include <algorithm>
#include <cassert>
#include <cstring>

UBO uniformBufferData;

void Scene::CreateGPUBuffers() {
    uniformBuffer = UniformBuffer::Create(0, sizeof(uniformBufferData));
    kdTreeSSBO = SSBO::Create(1);
    kdTreeIndicesSSBO = SSBO::Create(2);
    meshTrianglesSSBO = SSBO::Create(3);

    primitiveSSBO = SSBO::Create(10);
    sphereSSBO = SSBO::Create(11);
    triangleSSBO = SSBO::Create(12);
    planeSSBO = SSBO::Create(13);
    boxSSBO = SSBO::Create(14);
    meshSSBO = SSBO::Create(15);

    shaderSSBO = SSBO::Create(20);
    flatSSBO = SSBO::Create(21);
    refractionSSBO = SSBO::Create(22);
    mirrorSSBO = SSBO::Create(23);
    simpleShadowSSBO = SSBO::Create(24);
    lambertSSBO = SSBO::Create(25);
    phongSSBO = SSBO::Create(26);
    cookTorranceSSBO = SSBO::Create(27);
    simpleTextureSSBO = SSBO::Create(28);
    brdfSSBO = SSBO::Create(29);
    materialSSBO = SSBO::Create(30);
    emissiveSSBO = SSBO::Create(31);

    lightSSBO = SSBO::Create(40);
    pointSSBO = SSBO::Create(41);
    ambientSSBO = SSBO::Create(42);
    spotSSBO = SSBO::Create(43);
}

void Scene::UploadMeshTrianglesToGPU() {
    std::vector<std::shared_ptr<Triangle>> allMeshTris;
    for (const auto& It : m_Primitives) {
        if (It->type == PrimitiveType::Mesh) {
            Mesh* mesh = (Mesh*)It.get();
            mesh->minBounds_index.w = allMeshTris.size();
            mesh->maxBounds_count.w = mesh->m_Triangles.size();
            allMeshTris.insert(allMeshTris.end(), mesh->m_Triangles.begin(), mesh->m_Triangles.end());
        }
    }
    WriteBufferForType(allMeshTris, PrimitiveType::Triangle, *meshTrianglesSSBO);
}

void Scene::ConvertSceneToGPUData() {
    WriteBufferForType(m_Primitives, PrimitiveType::Sphere, *sphereSSBO);
    WriteBufferForType(m_Primitives, PrimitiveType::Triangle, *triangleSSBO);
    WriteBufferForType(m_Primitives, PrimitiveType::InfinitePlane, *planeSSBO);
    WriteBufferForType(m_Primitives, PrimitiveType::Box, *boxSSBO);
    WriteBufferForType(m_Primitives, PrimitiveType::Mesh, *meshSSBO);

    WriteBufferForType(m_Shaders, ShaderType::FlatShader, *flatSSBO);
    WriteBufferForType(m_Shaders, ShaderType::MirrorShader, *mirrorSSBO);
    WriteBufferForType(m_Shaders, ShaderType::RefractionShader, *refractionSSBO);
    WriteBufferForType(m_Shaders, ShaderType::SimpleShadowShader, *simpleShadowSSBO);
    WriteBufferForType(m_Shaders, ShaderType::LambertShader, *lambertSSBO);
    WriteBufferForType(m_Shaders, ShaderType::PhongShader, *phongSSBO);
    WriteBufferForType(m_Shaders, ShaderType::CookTorranceShader, *cookTorranceSSBO);
    WriteBufferForType(m_Shaders, ShaderType::SimpleTextureShader, *simpleTextureSSBO);
    WriteBufferForType(m_Shaders, ShaderType::MaterialShader, *materialSSBO);
    WriteBufferForType(m_Shaders, ShaderType::BrdfShader, *brdfSSBO);
    WriteBufferForType(m_Shaders, ShaderType::EmissiveShader, *emissiveSSBO);

    WriteBufferForType(m_Lights, LightType::PointLight, *pointSSBO);
    WriteBufferForType(m_Lights, LightType::AmbientLight, *ambientSSBO);
    WriteBufferForType(m_Lights, LightType::SpotLight, *spotSSBO);

    // PRIMITIVES BUFFER
    struct GPUPrimitive {
        uint32_t primitiveType;
        int32_t  primitiveIndex;
        uint32_t shaderType;
        int32_t  shaderIndex;
    };
    size_t GPUDataSize = 4 + sizeof(GPUPrimitive) * m_Primitives.size();
    void* primitiveDataGPU = primitiveSSBO->MapData(GPUDataSize);

    const uint32_t primitiveCount = static_cast<uint32_t>(m_Primitives.size());
    std::memcpy(primitiveDataGPU, &primitiveCount, sizeof(uint32_t));

    GPUPrimitive* primDst = reinterpret_cast<GPUPrimitive*>(
        reinterpret_cast<byte*>(primitiveDataGPU) + 4);

    for (size_t i = 0; i < m_Primitives.size(); ++i) {
        RT_ASSERT(m_Primitives[i]->type != PrimitiveType::None, "Primitive type is None");
        m_Primitives[i]->globalIndex = static_cast<int32_t>(i); // Set global index for KD-tree
        primDst[i].primitiveType = static_cast<uint32_t>(m_Primitives[i]->type);
        primDst[i].primitiveIndex = m_Primitives[i]->index;
        primDst[i].shaderType = static_cast<uint32_t>(m_Primitives[i]->shader->type);
        primDst[i].shaderIndex = m_Primitives[i]->shader->index;
    }

    primitiveSSBO->UnmapData();

    // LIGHTS BUFFER
    struct GPULight {
        uint32_t lightType;
        int32_t  lightIndex;
    };
    size_t GPULightDataSize = 4 + sizeof(GPULight) * m_Lights.size();
    void* lightDataGPU = lightSSBO->MapData(GPULightDataSize);

    const uint32_t lightCount = static_cast<uint32_t>(m_Lights.size());
    std::memcpy(lightDataGPU, &lightCount, sizeof(uint32_t));

    GPULight* lightDst = reinterpret_cast<GPULight*>(
        reinterpret_cast<byte*>(lightDataGPU) + 4);

    for (size_t i = 0; i < m_Lights.size(); ++i) {
        RT_ASSERT(m_Lights[i]->type != LightType::None, "Light type is None");
        lightDst[i].lightType = static_cast<uint32_t>(m_Lights[i]->type);
        lightDst[i].lightIndex = m_Lights[i]->index;
    }

    lightSSBO->UnmapData();
}

void Scene::UploadKDTreeToGPU() {
    if (m_KDTree.IsEmpty()) {
        // Upload minimal data for empty tree
        struct {
            Vec4 boundsMin;
            Vec4 boundsMax;
            uint32_t nodeCount;
            uint32_t padding[3];
        } emptyHeader = {};
        void* data = kdTreeSSBO->MapData(sizeof(emptyHeader));
        std::memcpy(data, &emptyHeader, sizeof(emptyHeader));
        kdTreeSSBO->UnmapData();

        // Also upload empty indices buffer
        struct {
            uint32_t indexCount;
            uint32_t padding[3];
        } emptyIndices = {};
        void* indicesData = kdTreeIndicesSSBO->MapData(sizeof(emptyIndices));
        std::memcpy(indicesData, &emptyIndices, sizeof(emptyIndices));
        kdTreeIndicesSSBO->UnmapData();
        return;
    }

    const auto& nodes = m_KDTree.GetNodes();
    const auto& indices = m_KDTree.GetPrimitiveIndices();
    const Vec3& boundsMin = m_KDTree.GetBoundsMin();
    const Vec3& boundsMax = m_KDTree.GetBoundsMax();

    // KD-tree nodes SSBO layout:
    // vec4 boundsMin
    // vec4 boundsMax
    // uint nodeCount
    // uint padding[3]
    // GPUKDNode nodes[]
    size_t headerSize = sizeof(Vec4) * 2 + sizeof(uint32_t) * 4;
    size_t nodesSize = sizeof(GPUKDNode) * nodes.size();
    size_t totalSize = headerSize + nodesSize;

    void* data = kdTreeSSBO->MapData(totalSize);
    byte* ptr = static_cast<byte*>(data);

    // Write bounds
    Vec4 min4(boundsMin, 0.0f);
    Vec4 max4(boundsMax, 0.0f);
    std::memcpy(ptr, &min4, sizeof(Vec4));
    ptr += sizeof(Vec4);
    std::memcpy(ptr, &max4, sizeof(Vec4));
    ptr += sizeof(Vec4);

    // Write node count
    uint32_t nodeCount = static_cast<uint32_t>(nodes.size());
    std::memcpy(ptr, &nodeCount, sizeof(uint32_t));
    ptr += sizeof(uint32_t) * 4;  // count + padding

    // Write nodes
    std::memcpy(ptr, nodes.data(), nodesSize);

    kdTreeSSBO->UnmapData();

    // Primitive indices SSBO layout:
    // uint indexCount
    // uint padding[3]
    // int indices[]
    size_t indicesHeaderSize = sizeof(uint32_t) * 4;
    size_t indicesDataSize = sizeof(int) * indices.size();
    size_t indicesTotalSize = indicesHeaderSize + indicesDataSize;

    void* indicesData = kdTreeIndicesSSBO->MapData(indicesTotalSize);
    ptr = static_cast<byte*>(indicesData);

    uint32_t indexCount = static_cast<uint32_t>(indices.size());
    std::memcpy(ptr, &indexCount, sizeof(uint32_t));
    ptr += indicesHeaderSize;

    if (!indices.empty()) {
        std::memcpy(ptr, indices.data(), indicesDataSize);
    }

    kdTreeIndicesSSBO->UnmapData();
}

void Scene::UpdateGPUBuffers() {
    // Always update uniform buffer with render resolution (independent of window size)
    uniformBufferData.u_resolution = glm::vec2(Params::GetWidth(), Params::GetHeight());
    uniformBufferData.u_aspectRatio = uniformBufferData.u_resolution.y / uniformBufferData.u_resolution.x;
    uniformBufferData.u_Seed = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    uniformBuffer->UploadData(&uniformBufferData, sizeof(uniformBufferData));
    uniformBufferData.u_SampleIndex++;

    if (IsBufferDirty()) {
        UploadMeshTrianglesToGPU();
        ConvertSceneToGPUData();  // Must set primitive indices before building KD-tree
        BuildKDTree();
        UploadKDTreeToGPU();
        SetBufferDirty(false);
    }
}

void Scene::BuildKDTree() {
    m_KDTree.BuildTree(m_Primitives);
}

void Scene::ClearScene() {
    uniformBufferData.u_SampleIndex = 0;
    m_Primitives.clear();
    m_Shaders.clear();
    m_Lights.clear();
    m_IsBufferDirty = true;
}
