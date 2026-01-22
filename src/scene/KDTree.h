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
    void BuildTree(const std::vector<std::shared_ptr<Primitive>>& primitives, int maxDepth = 20, int minPrimitives = 2);

    // Get flattened data for GPU upload
    const std::vector<GPUKDNode>& GetNodes() const { return m_Nodes; }
    const std::vector<int>& GetPrimitiveIndices() const { return m_PrimitiveIndices; }
    const Vec3& GetBoundsMin() const { return absoluteMinimum; }
    const Vec3& GetBoundsMax() const { return absoluteMaximum; }
    bool IsEmpty() const { return m_Nodes.empty(); }

private:
    struct Node {
        std::unique_ptr<Node> child[2];
        int dimension = 0;
        float split = 0;
        std::vector<std::shared_ptr<Primitive>> primitives;

        inline bool IsLeaf() const { return (!this->primitives.empty() || (!this->child[0] && !this->child[1])); }
    };

    std::unique_ptr<Node> Build(Vec3 const &minimumBounds, Vec3 const &maximumBounds, const std::vector<std::shared_ptr<Primitive>> &primitives, int depth);
    int FlattenTree(Node* node);

    // Build-time reference to primitives
    int maximumDepth, minimumNumberOfPrimitives;

    // Scene bounds
    Vec3 absoluteMinimum, absoluteMaximum;

    // Flattened GPU data
    std::vector<GPUKDNode> m_Nodes;
    std::vector<int> m_PrimitiveIndices;
};

#endif
