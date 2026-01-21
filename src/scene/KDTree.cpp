#include "scene/KDTree.h"
#include "common/Log.h"
#include <algorithm>
#include <limits>

void KDTree::Build(const std::vector<std::shared_ptr<Primitive>>& primitives,
                   int maxDepth, int minPrimitives) {
    m_Primitives = &primitives;
    m_MaxDepth = maxDepth;
    m_MinPrimitives = minPrimitives;
    m_Nodes.clear();
    m_PrimitiveIndices.clear();

    if (primitives.empty()) {
        m_BoundsMin = Vec3(0.0f);
        m_BoundsMax = Vec3(0.0f);
        return;
    }

    // Compute world bounds from all primitives
    // Note: Don't clamp infinities - let infinite planes have infinite bounds
    // The reference implementation uses ±INFINITY directly
    m_BoundsMin = Vec3(std::numeric_limits<float>::infinity());
    m_BoundsMax = Vec3(-std::numeric_limits<float>::infinity());

    for (const auto& prim : primitives) {
        for (int d = 0; d < 3; ++d) {
            float pmin = prim->minimumBounds(d);
            float pmax = prim->maximumBounds(d);
            m_BoundsMin[d] = std::min(m_BoundsMin[d], pmin);
            m_BoundsMax[d] = std::max(m_BoundsMax[d], pmax);
        }
    }

    // Build initial primitive index list
    std::vector<int> allIndices(primitives.size());
    for (size_t i = 0; i < primitives.size(); ++i) {
        allIndices[i] = static_cast<int>(i);
    }

    // Build tree recursively
    auto root = BuildRecursive(m_BoundsMin, m_BoundsMax, allIndices, 0);

    // Flatten for GPU
    if (root) {
        FlattenTree(root.get());
    }

    // RT_INFO("KD-tree built: {0} nodes, {1} primitive references", m_Nodes.size(), m_PrimitiveIndices.size());
}

std::unique_ptr<KDTree::BuildNode> KDTree::BuildRecursive(
    const Vec3& minBounds, const Vec3& maxBounds,
    const std::vector<int>& primIndices,
    int depth) {

    auto node = std::make_unique<BuildNode>();

    // Compute bounding box diameter
    Vec3 diameter = maxBounds - minBounds;

    // Find widest and narrowest dimensions
    int widestDim = (diameter.x > diameter.y)
        ? ((diameter.x > diameter.z) ? 0 : 2)
        : ((diameter.y > diameter.z) ? 1 : 2);

    int narrowestDim = (diameter.x < diameter.y)
        ? ((diameter.x < diameter.z) ? 0 : 2)
        : ((diameter.y < diameter.z) ? 1 : 2);

    // Check termination conditions
    bool isLeaf = (depth >= m_MaxDepth) ||
                  (static_cast<int>(primIndices.size()) <= m_MinPrimitives) ||
                  (diameter[narrowestDim] <= EPSILON);

    if (isLeaf) {
        node->primitiveIndices = primIndices;
        return node;
    }

    // Split along widest dimension
    node->dimension = widestDim;

    // Compute split position using median of minimum bounds
    // Note: Reference implementation doesn't filter infinite values
    std::vector<float> minValues;
    minValues.reserve(primIndices.size());
    for (int idx : primIndices) {
        minValues.push_back((*m_Primitives)[idx]->minimumBounds(widestDim));
    }
    std::sort(minValues.begin(), minValues.end());
    node->split = minValues[minValues.size() / 2];

    // Partition primitives into left and right
    // A primitive goes LEFT if its minBounds < split
    // A primitive goes RIGHT if its maxBounds >= split
    // Primitives spanning the split go in BOTH
    std::vector<int> leftPrims, rightPrims;

    for (int idx : primIndices) {
        float pmin = (*m_Primitives)[idx]->minimumBounds(widestDim);
        float pmax = (*m_Primitives)[idx]->maximumBounds(widestDim);

        if (pmin < node->split) {
            leftPrims.push_back(idx);
        }
        if (pmax >= node->split) {
            rightPrims.push_back(idx);
        }
    }

    // Check for degenerate splits (all primitives on one side)
    if (leftPrims.empty() || rightPrims.empty()) {
        // Fall back to leaf
        node->dimension = -1;
        node->primitiveIndices = primIndices;
        return node;
    }

    // Check if we're not making progress (same primitives in both children)
    if (leftPrims.size() == primIndices.size() && rightPrims.size() == primIndices.size()) {
        // All primitives span the split - make leaf
        node->dimension = -1;
        node->primitiveIndices = primIndices;
        return node;
    }

    // Compute child bounds
    Vec3 leftMax = maxBounds;
    leftMax[widestDim] = node->split;

    Vec3 rightMin = minBounds;
    rightMin[widestDim] = node->split;

    // Recursively build children
    node->children[0] = BuildRecursive(minBounds, leftMax, leftPrims, depth + 1);
    node->children[1] = BuildRecursive(rightMin, maxBounds, rightPrims, depth + 1);

    return node;
}

int KDTree::FlattenTree(BuildNode* node) {
    int nodeIndex = static_cast<int>(m_Nodes.size());
    m_Nodes.emplace_back();

    if (node->IsLeaf()) {
        // For leaf nodes, we can fill in all fields immediately
        m_Nodes[nodeIndex].childLeft = -1;
        m_Nodes[nodeIndex].childRight = -1;
        m_Nodes[nodeIndex].dimension = -1;
        m_Nodes[nodeIndex].split = 0.0f;
        m_Nodes[nodeIndex].primStart = static_cast<int>(m_PrimitiveIndices.size());
        m_Nodes[nodeIndex].primCount = static_cast<int>(node->primitiveIndices.size());

        // Add primitive indices
        for (int idx : node->primitiveIndices) {
            m_PrimitiveIndices.push_back(idx);
        }
    } else {
        // For internal nodes, store data first, then recurse
        // (recursion may reallocate the vector, invalidating references)
        m_Nodes[nodeIndex].dimension = node->dimension;
        m_Nodes[nodeIndex].split = node->split;
        m_Nodes[nodeIndex].primStart = 0;
        m_Nodes[nodeIndex].primCount = 0;

        // Flatten children - must use index access since vector may reallocate
        int leftChild = FlattenTree(node->children[0].get());
        int rightChild = FlattenTree(node->children[1].get());
        m_Nodes[nodeIndex].childLeft = leftChild;
        m_Nodes[nodeIndex].childRight = rightChild;
    }

    return nodeIndex;
}
