#include "scene/KDTree.h"
#include "common/Log.h"
#include <algorithm>
#include <limits>

void KDTree::BuildTree(const std::vector<std::shared_ptr<Primitive>>& primitives, int maxDepth, int minPrimitives) {
    this->maximumDepth = maxDepth;
    this->minimumNumberOfPrimitives = minPrimitives;
    m_Nodes.clear();
    m_PrimitiveIndices.clear();

  // Determine the bounding box of the kD-Tree
    this->absoluteMinimum = Vec3(+INFINITY, +INFINITY, +INFINITY);
    this->absoluteMaximum = Vec3(-INFINITY, -INFINITY, -INFINITY);
    for (const auto &primitive : primitives) {
        for (int d = 0; d < 3; ++d) {
            this->absoluteMinimum[d] = std::min(this->absoluteMinimum[d], primitive->minimumBounds(d));
            this->absoluteMaximum[d] = std::max(this->absoluteMaximum[d], primitive->maximumBounds(d));
        }
    }

    // Build tree recursively
    auto root = Build(absoluteMinimum, absoluteMaximum, primitives, 0);

    // Flatten for GPU
    if (root) {
        FlattenTree(root.get());
    }

    RT_INFO("KD-tree built: {0} nodes, {1} primitive references", m_Nodes.size(), m_PrimitiveIndices.size());
}

std::unique_ptr<KDTree::Node> KDTree::Build(Vec3 const &minimumBounds, Vec3 const &maximumBounds, const std::vector<std::shared_ptr<Primitive>> &primitives, int depth) {
    // Determine the diameter of the bounding box
    Vec3 const diameter = maximumBounds - minimumBounds;

    // Test whether we have reached a leaf node...
    int minimumDimension = ((diameter.x < diameter.y) ? ((diameter.x < diameter.z) ? 0 : 2) : ((diameter.y < diameter.z) ? 1 : 2));
    if (depth >= this->maximumDepth || (int)primitives.size() <= this->minimumNumberOfPrimitives || (diameter[minimumDimension]) <= EPSILON) {
        auto leafNode = std::make_unique<Node>();
        leafNode->primitives = primitives;
        return leafNode;
    }

    // ... otherwise create a new inner node by splitting through the widest dimension
    auto node = std::make_unique<Node>();
    node->dimension = ((diameter.x > diameter.y) ? ((diameter.x > diameter.z) ? 0 : 2) : ((diameter.y > diameter.z) ? 1 : 2));

    // Determine the split position
    // Note: Use the median of the minimum bounds of the primitives
    std::vector<float> minimumValues;
    for (const auto &primitive : primitives)
        minimumValues.push_back(primitive->minimumBounds(node->dimension));
    std::sort(minimumValues.begin(), minimumValues.end());
    node->split = minimumValues[minimumValues.size() / 2];

    // Divide primitives into the left and right lists
    // Remember: A primitive can be in both lists!
    // Also remember: You split exactly at the minimum of a primitive,
    // make sure *that* primitive does *not* appear in both lists!
    std::vector<std::shared_ptr<Primitive>> leftPrimitives, rightPrimitives;
    for (const auto &primitive : primitives) {
        if (primitive->minimumBounds(node->dimension) < node->split)
            leftPrimitives.push_back(primitive);
        if (primitive->maximumBounds(node->dimension) >= node->split)
            rightPrimitives.push_back(primitive);
    }

    // Print out the number of primitives in the left and right child node
    // std::cout << "(FastScene): Split " << leftPrimitives.size() << " | " << rightPrimitives.size() << std::endl;

    // Set the left and right split vectors
    Vec3 minimumSplit = minimumBounds;
    Vec3 maximumSplit = maximumBounds;
    minimumSplit[node->dimension] = node->split;
    maximumSplit[node->dimension] = node->split;

    // Recursively build the tree
    depth += 1;
    node->child[0] = this->Build(minimumBounds, maximumSplit, leftPrimitives, depth);
    node->child[1] = this->Build(minimumSplit, maximumBounds, rightPrimitives, depth);
    return node;
}

int KDTree::FlattenTree(Node* node) {
    int nodeIndex = static_cast<int>(m_Nodes.size());
    m_Nodes.emplace_back();

    if (node->IsLeaf()) {
        // For leaf nodes, we can fill in all fields immediately
        m_Nodes[nodeIndex].childLeft = -1;
        m_Nodes[nodeIndex].childRight = -1;
        m_Nodes[nodeIndex].dimension = -1;
        m_Nodes[nodeIndex].split = 0.0f;
        m_Nodes[nodeIndex].primStart = static_cast<int>(m_PrimitiveIndices.size());
        m_Nodes[nodeIndex].primCount = static_cast<int>(node->primitives.size());

        // Add primitive indices
        for (auto prim : node->primitives) {
            m_PrimitiveIndices.push_back(prim->globalIndex);
        }
    } else {
        // For internal nodes, store data first, then recurse
        // (recursion may reallocate the vector, invalidating references)
        m_Nodes[nodeIndex].dimension = node->dimension;
        m_Nodes[nodeIndex].split = node->split;
        m_Nodes[nodeIndex].primStart = 0;
        m_Nodes[nodeIndex].primCount = 0;

        // Flatten children - must use index access since vector may reallocate
        int leftChild = FlattenTree(node->child[0].get());
        int rightChild = FlattenTree(node->child[1].get());
        m_Nodes[nodeIndex].childLeft = leftChild;
        m_Nodes[nodeIndex].childRight = rightChild;
    }

    return nodeIndex;
}
