#ifndef KDTREE_H
#define KDTREE_H

#include "common/Types.h"
#include "primitives/Primitive.h"
#include <vector>
#include <memory>

// GPU-friendly node structure
// If childLeft == -1, this is a leaf node and primStart/primCount are valid
// Otherwise, childLeft and childRight point to children
struct GPUKDNode {
    int childLeft;      // -1 for leaf
    int childRight;     // -1 for leaf
    int dimension;      // split axis (0, 1, 2)
    float split;        // split plane position
    int primStart;      // start index in primitive index array (leaf only)
    int primCount;      // number of primitives (leaf only)
    int _pad[2];        // padding to 32 bytes
};

class KDTree {
public:
    KDTree() = default;
    ~KDTree() = default;

    // Build the tree from primitives
    void Build(const std::vector<std::shared_ptr<Primitive>>& primitives,
               int maxDepth = 20, int minPrimitives = 2);

    // Get flattened data for GPU upload
    const std::vector<GPUKDNode>& GetNodes() const { return m_Nodes; }
    const std::vector<int>& GetPrimitiveIndices() const { return m_PrimitiveIndices; }
    const Vec3& GetBoundsMin() const { return m_BoundsMin; }
    const Vec3& GetBoundsMax() const { return m_BoundsMax; }
    bool IsEmpty() const { return m_Nodes.empty(); }

private:
    struct BuildNode {
        std::unique_ptr<BuildNode> children[2];
        int dimension = -1;
        float split = 0.0f;
        std::vector<int> primitiveIndices;  // indices into original primitive array

        bool IsLeaf() const { return children[0] == nullptr && children[1] == nullptr; }
    };

    std::unique_ptr<BuildNode> BuildRecursive(
        const Vec3& minBounds, const Vec3& maxBounds,
        const std::vector<int>& primIndices,
        int depth);

    int FlattenTree(BuildNode* node);

    // Build-time reference to primitives
    const std::vector<std::shared_ptr<Primitive>>* m_Primitives = nullptr;
    int m_MaxDepth = 20;
    int m_MinPrimitives = 2;

    // Scene bounds
    Vec3 m_BoundsMin;
    Vec3 m_BoundsMax;

    // Flattened GPU data
    std::vector<GPUKDNode> m_Nodes;
    std::vector<int> m_PrimitiveIndices;
};

#endif
