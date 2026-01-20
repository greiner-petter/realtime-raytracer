#include "scene/Scene.h"
#include "common/Log.h"
#include "common/Window.h"
#include "common/Params.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>
#include <vector>
#include <cmath>

// ============================================================================
// Left-Balanced KD-Tree Helper Functions (from Wald's paper)
// ============================================================================

// Count leading zeros (pure C++ implementation)
static inline int clz(uint32_t x) {
    if (x == 0) return 32;
    int n = 0;
    if ((x & 0xFFFF0000) == 0) { n += 16; x <<= 16; }
    if ((x & 0xFF000000) == 0) { n += 8;  x <<= 8; }
    if ((x & 0xF0000000) == 0) { n += 4;  x <<= 4; }
    if ((x & 0xC0000000) == 0) { n += 2;  x <<= 2; }
    if ((x & 0x80000000) == 0) { n += 1; }
    return n;
}

// Tree level of node i: level(i) = floor(log2(i+1))
static inline int nodeLevel(int i) {
    return 31 - clz(static_cast<uint32_t>(i + 1));
}

// Number of levels in tree of N nodes
static inline int numLevels(int N) {
    if (N <= 0) return 0;
    return nodeLevel(N - 1) + 1;
}

// Left child of node i
static inline int lChild(int i) {
    return 2 * i + 1;
}

// Right child of node i
static inline int rChild(int i) {
    return 2 * i + 2;
}

// Number of nodes in a full tree of l levels: F(l) = 2^l - 1
static inline int fullTreeSize(int l) {
    return (1 << l) - 1;
}

// Subtree size under node s in a tree of N nodes
// Based on Wald's paper Section 4.1
static int subtreeSize(int s, int N) {
    if (s >= N) return 0;

    int L = numLevels(N);
    int l = nodeLevel(s);

    // First lowest-level child of subtree s
    // fllc_s = ~((~s) << (L-l-1))
    int shift = L - l - 1;
    if (shift < 0) shift = 0;
    int fllc_s = ~((~s) << shift);

    // Inner nodes (levels l+1 to L-2, all full)
    int innerNodes = (1 << shift) - 1;

    // Nodes on lowest level
    int maxLowestLevel = 1 << shift;
    int lowestLevelNodes = std::min(std::max(0, N - fllc_s), maxLowestLevel);

    return innerNodes + lowestLevelNodes;
}

// Segment begin index for subtree s in step l
// Based on Wald's paper Section 4.2
static int segmentBegin(int s, int l, int N) {
    int L = numLevels(N);

    // Number of left siblings: nls = s - F(l) = s - (2^l - 1)
    int F_l = fullTreeSize(l);
    int nls_s = s - F_l;

    // Calculate segment begin
    int shift = L - l - 1;
    if (shift < 0) shift = 0;

    // Top l levels (already fixed)
    int topLevels = F_l;

    // Left siblings' inner nodes
    int leftSiblingsInner = nls_s * ((1 << shift) - 1);

    // Left siblings' lowest level nodes
    int lowestLevelStart = fullTreeSize(L - 1);  // F(L-1) = 2^(L-1) - 1
    int leftSiblingsLowest = std::min(nls_s * (1 << shift),
                                       std::max(0, N - lowestLevelStart));

    return topLevels + leftSiblingsInner + leftSiblingsLowest;
}

// ============================================================================
// Scene Implementation
// ============================================================================

UBO uniformBufferData;

void Scene::CreateGPUBuffers() {
    uniformBuffer = UniformBuffer::Create(0, sizeof(uniformBufferData));
    kdTreeSSBO = SSBO::Create(1);

    primitiveSSBO = SSBO::Create(10);
    sphereSSBO = SSBO::Create(11);
    triangleSSBO = SSBO::Create(12);
    planeSSBO = SSBO::Create(13);
    boxSSBO = SSBO::Create(14);

    shaderSSBO = SSBO::Create(20);
    flatSSBO = SSBO::Create(21);
    refractionSSBO = SSBO::Create(22);
    mirrorSSBO = SSBO::Create(23);
    simpleShadowSSBO = SSBO::Create(24);
    lambertSSBO = SSBO::Create(25);
    phongSSBO = SSBO::Create(26);
    simpleTextureSSBO = SSBO::Create(27);

    lightSSBO = SSBO::Create(30);
    pointSSBO = SSBO::Create(31);
    ambientSSBO = SSBO::Create(32);
    spotSSBO = SSBO::Create(33);
}

void Scene::ConvertSceneToGPUData() {
    WriteBufferForType(m_Primitives, PrimitiveType::Sphere, *sphereSSBO);
    WriteBufferForType(m_Primitives, PrimitiveType::Triangle, *triangleSSBO);
    WriteBufferForType(m_Primitives, PrimitiveType::InfinitePlane, *planeSSBO);
    WriteBufferForType(m_Primitives, PrimitiveType::Box, *boxSSBO);

    WriteBufferForType(m_Shaders, ShaderType::FlatShader, *flatSSBO);
    WriteBufferForType(m_Shaders, ShaderType::MirrorShader, *mirrorSSBO);
    WriteBufferForType(m_Shaders, ShaderType::RefractionShader, *refractionSSBO);
    WriteBufferForType(m_Shaders, ShaderType::SimpleShadowShader, *simpleShadowSSBO);
    WriteBufferForType(m_Shaders, ShaderType::LambertShader, *lambertSSBO);
    WriteBufferForType(m_Shaders, ShaderType::PhongShader, *phongSSBO);
    WriteBufferForType(m_Shaders, ShaderType::SimpleTextureShader, *simpleTextureSSBO);

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
    // Upload scene bounds only (tree structure is implicit)
    size_t GPUDataSize = sizeof(Vec4) * 2;

    void* DataGPU = kdTreeSSBO->MapData(GPUDataSize);

    std::memcpy(DataGPU, &absoluteMinimum, sizeof(Vec4));
    std::memcpy(static_cast<byte*>(DataGPU) + sizeof(Vec4), &absoluteMaximum, sizeof(Vec4));

    kdTreeSSBO->UnmapData();
}

void Scene::UpdateGPUBuffers() {
    // Always update uniform buffer
    uniformBufferData.u_resolution = Params::IsInteractiveMode()
        ? glm::vec2(Window::GetInstance()->GetWidth(), Window::GetInstance()->GetHeight())
        : glm::vec2(Params::GetWidth(), Params::GetHeight());

    uniformBufferData.u_aspectRatio = uniformBufferData.u_resolution.y / uniformBufferData.u_resolution.x;
    uniformBufferData.u_Seed = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    uniformBuffer->UploadData(&uniformBufferData, sizeof(uniformBufferData));
    uniformBufferData.u_SampleIndex++;

    if (IsBufferDirty()) {
        BuildKDTree();
        UploadKDTreeToGPU();
        ConvertSceneToGPUData();
        SetBufferDirty(false);
    }
}

// Build left-balanced KD-tree using Wald's algorithm
// Sorts primitives in level-order so tree topology is implicit from indices
void Scene::BuildKDTree() {
    int N = static_cast<int>(m_Primitives.size());

    if (N == 0) {
        absoluteMinimum = Vec4(0.0f);
        absoluteMaximum = Vec4(0.0f);
        return;
    }

    // Compute scene bounds
    absoluteMinimum = Vec4(+INFINITY, +INFINITY, +INFINITY, 0.0f);
    absoluteMaximum = Vec4(-INFINITY, -INFINITY, -INFINITY, 0.0f);
    for (const auto& primitive : m_Primitives) {
        for (int d = 0; d < 3; ++d) {
            absoluteMinimum[d] = std::min(absoluteMinimum[d], primitive->minimumBounds(d));
            absoluteMaximum[d] = std::max(absoluteMaximum[d], primitive->maximumBounds(d));
        }
    }

    // Special case: single primitive
    if (N == 1) {
        RT_INFO("{0} primitive organized into tree", N);
        return;
    }

    int treeN = N;

    // Initialize tags to 0 (level-0 ancestor = root)
    std::vector<int> tags(treeN, 0);

    int L = numLevels(treeN);

    // Helper to get centroid of primitive along axis
    auto getCentroid = [this](int idx, int axis) -> float {
        return 0.5f * (m_Primitives[idx]->minimumBounds(axis) +
                       m_Primitives[idx]->maximumBounds(axis));
    };

    // Iterate through tree levels (only for finite primitives)
    for (int l = 0; l < L; ++l) {
        int dim = l % 3;  // Split dimension for this level

        // Create index array for sorting (only finite primitives)
        std::vector<int> indices(treeN);
        std::iota(indices.begin(), indices.end(), 0);

        // Sort by (tag, centroid[dim], index) - proper strict weak ordering
        std::sort(indices.begin(), indices.end(), [&](int a, int b) {
            int ta = tags[a];
            int tb = tags[b];
            if (ta < tb) return true;
            if (tb < ta) return false;

            float ca = getCentroid(a, dim);
            float cb = getCentroid(b, dim);
            if (ca < cb) return true;
            if (cb < ca) return false;

            return a < b;  // tiebreaker
        });

        // Apply permutation to primitives and tags
        std::vector<std::shared_ptr<Primitive>> newPrimitives(treeN);
        std::vector<int> newTags(treeN);
        for (int i = 0; i < treeN; ++i) {
            newPrimitives[i] = m_Primitives[indices[i]];
            newTags[i] = tags[indices[i]];
        }
        for (int i = 0; i < treeN; ++i) {
            m_Primitives[i] = newPrimitives[i];
        }
        tags = std::move(newTags);

        // Update tags for elements not yet fixed
        int F_l = fullTreeSize(l);  // Elements already at final positions
        for (int i = F_l; i < treeN; ++i) {
            int s = tags[i];  // Current subtree this element is in

            // Calculate pivot position for subtree s
            int leftChildSize = subtreeSize(lChild(s), treeN);
            int pivotPos = segmentBegin(s, l, treeN) + leftChildSize;

            if (i < pivotPos) {
                // Goes to left subtree
                tags[i] = lChild(s);
            } else if (i > pivotPos) {
                // Goes to right subtree
                tags[i] = rChild(s);
            }
            // else: stays at s (this element becomes the root of subtree s)
        }
    }

    RT_INFO("{0} primitives organized into left-balanced KD-tree", N);
}

void Scene::ClearScene() {
    uniformBufferData.u_SampleIndex = 0;
    m_Primitives.clear();
    m_Shaders.clear();
    m_Lights.clear();
    m_IsBufferDirty = true;
}
