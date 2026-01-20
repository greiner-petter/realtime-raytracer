// Left-balanced KD-tree traversal
// Tree topology is implicit from array indices:
// - lChild(i) = 2*i + 1
// - rChild(i) = 2*i + 2
// - axis = level(i) % 3
// - split value = centroid of primitive[i] along axis

layout(binding = 1, std430) buffer kdTree {
    vec4 sceneBoundsMin;
    vec4 sceneBoundsMax;
};

// KD-tree traversal for intersection
bool intersectKDTree(inout Ray ray) {
    int N = int(primitiveCount);
    if (N == 0) return false;

    bool hitAny = false;

    // Stack for traversal (just node indices, no intervals needed for full traversal)
    int stack[MAX_STACK];
    int sp = 0;

    // Push root
    stack[sp++] = 0;

    while (sp > 0) {
        int nodeIdx = stack[--sp];

        // Check if this is a valid node
        if (nodeIdx >= N) continue;

        // Intersect with the primitive at this node
        if (intersect(ray, primitives[nodeIdx])) {
            hitAny = true;
        }

        // Push children (always traverse both)
        int leftChild = 2 * nodeIdx + 1;
        int rightChild = 2 * nodeIdx + 2;

        if (rightChild < N) stack[sp++] = rightChild;
        if (leftChild < N) stack[sp++] = leftChild;
    }

    return hitAny;
}

// KD-tree traversal for occlusion testing (early out on first hit)
bool occludeKDTree(inout Ray ray) {
    int N = int(primitiveCount);
    if (N == 0) return false;

    // Stack for traversal
    int stack[MAX_STACK];
    int sp = 0;

    // Push root
    stack[sp++] = 0;

    while (sp > 0) {
        int nodeIdx = stack[--sp];

        // Check if this is a valid node
        if (nodeIdx >= N) continue;

        // Intersect with the primitive at this node - early out on hit
        if (intersect(ray, primitives[nodeIdx])) {
            return true;
        }

        // Push children (always traverse both)
        int leftChild = 2 * nodeIdx + 1;
        int rightChild = 2 * nodeIdx + 2;

        if (rightChild < N) stack[sp++] = rightChild;
        if (leftChild < N) stack[sp++] = leftChild;
    }

    return false;
}
