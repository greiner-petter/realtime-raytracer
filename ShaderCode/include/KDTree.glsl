// KD-tree traversal using explicit node structure
// Based on reference implementation from group_8/scene/fastscene.cpp

layout(binding = 1, std430) buffer kdTree {
    vec4 sceneBoundsMin;
    vec4 sceneBoundsMax;
    uint nodeCount;
    uint _kdPadding[3];
    int nodeData[];
};

layout(binding = 2, std430) buffer kdTreeIndices {
    uint indexCount;
    uint _idxPadding[3];
    int primIndices[];
};

// Node accessor functions (8 ints per node = 32 bytes)
int getChildLeft(int nodeIdx) { return nodeData[nodeIdx * 8 + 0]; }
int getChildRight(int nodeIdx) { return nodeData[nodeIdx * 8 + 1]; }
int getDimension(int nodeIdx) { return nodeData[nodeIdx * 8 + 2]; }
float getSplit(int nodeIdx) { return intBitsToFloat(nodeData[nodeIdx * 8 + 3]); }
int getPrimStart(int nodeIdx) { return nodeData[nodeIdx * 8 + 4]; }
int getPrimCount(int nodeIdx) { return nodeData[nodeIdx * 8 + 5]; }
bool isLeaf(int nodeIdx) { return getChildLeft(nodeIdx) == -1; }

// KD-tree traversal for intersection
bool intersectKDTree(inout Ray ray) {
    if (nodeCount == 0) return false;

    // Bounding box intersection (matching reference implementation)
    vec3 invDir = 1.0 / ray.direction;
    vec3 tMin3 = (sceneBoundsMin.xyz - ray.origin) * invDir;
    vec3 tMax3 = (sceneBoundsMax.xyz - ray.origin) * invDir;
    vec3 t1 = min(tMin3, tMax3);
    vec3 t2 = max(tMin3, tMax3);
    float tMin = max(max(t1.x, t1.y), t1.z);
    float tMax = min(min(t2.x, t2.y), t2.z);

    // Check if ray intersects bounding box
    if (!(0.0 <= tMax && tMin <= tMax)) {
        return false;
    }

    bool hitAny = false;

    // Stack for iterative traversal
    int stackNode[MAX_STACK];
    float stackT0[MAX_STACK];
    float stackT1[MAX_STACK];
    int sp = 0;

    stackNode[sp] = 0;
    stackT0[sp] = tMin;
    stackT1[sp] = tMax;
    sp++;

    while (sp > 0) {
        sp--;
        int nodeIdx = stackNode[sp];
        float t0 = stackT0[sp];
        float t1 = stackT1[sp];

        // Skip this subtree if we already found a closer hit
        if (t0 > ray.rayLength) continue;

        if (isLeaf(nodeIdx)) {
            // Leaf node - intersect all primitives
            int start = getPrimStart(nodeIdx);
            int count = getPrimCount(nodeIdx);
            for (int i = 0; i < count; i++) {
                int primIdx = primIndices[start + i];
                if (intersect(ray, primitives[primIdx])) {
                    hitAny = true;
                }
            }
        } else {
            // Internal node
            int dim = getDimension(nodeIdx);
            float splitVal = getSplit(nodeIdx);
            float d = (splitVal - ray.origin[dim]) / ray.direction[dim];

            // Determine front/back based on ray direction (matching reference)
            int front = ray.direction[dim] < 0.0 ? 1 : 0;
            int back = 1 - front;
            int childFront = (front == 0) ? getChildLeft(nodeIdx) : getChildRight(nodeIdx);
            int childBack = (back == 0) ? getChildLeft(nodeIdx) : getChildRight(nodeIdx);

            if (d <= t0 || d < 0.0) {
                // t0..t1 is totally behind d, only traverse back
                if (childBack >= 0 && sp < MAX_STACK) {
                    stackNode[sp] = childBack;
                    stackT0[sp] = t0;
                    stackT1[sp] = t1;
                    sp++;
                }
            } else if (d >= t1) {
                // t0..t1 is totally in front of d, only traverse front
                if (childFront >= 0 && sp < MAX_STACK) {
                    stackNode[sp] = childFront;
                    stackT0[sp] = t0;
                    stackT1[sp] = t1;
                    sp++;
                }
            } else {
                // Traverse both - back first (will be processed second), then front
                if (childBack >= 0 && sp < MAX_STACK - 1) {
                    stackNode[sp] = childBack;
                    stackT0[sp] = d - SPLT_EPS;
                    stackT1[sp] = t1 + SPLT_EPS;
                    sp++;
                }
                if (childFront >= 0 && sp < MAX_STACK) {
                    stackNode[sp] = childFront;
                    stackT0[sp] = t0 - SPLT_EPS;
                    stackT1[sp] = d + SPLT_EPS;
                    sp++;
                }
            }
        }
    }

    return hitAny;
}

// KD-tree traversal for occlusion (early out on hit)
bool occludeKDTree(inout Ray ray) {
    if (nodeCount == 0) return false;

    // Bounding box intersection
    vec3 invDir = 1.0 / ray.direction;
    vec3 tMin3 = (sceneBoundsMin.xyz - ray.origin) * invDir;
    vec3 tMax3 = (sceneBoundsMax.xyz - ray.origin) * invDir;
    vec3 t1 = min(tMin3, tMax3);
    vec3 t2 = max(tMin3, tMax3);
    float tMin = max(max(t1.x, t1.y), t1.z);
    float tMax = min(min(t2.x, t2.y), t2.z);

    if (!(0.0 <= tMax && tMin <= tMax)) {
        return false;
    }

    // Stack for iterative traversal
    int stackNode[MAX_STACK];
    float stackT0[MAX_STACK];
    float stackT1[MAX_STACK];
    int sp = 0;

    stackNode[sp] = 0;
    stackT0[sp] = tMin;
    stackT1[sp] = tMax;
    sp++;

    while (sp > 0) {
        sp--;
        int nodeIdx = stackNode[sp];
        float t0 = stackT0[sp];
        float t1 = stackT1[sp];

        // Skip this subtree if beyond shadow ray length
        if (t0 > ray.rayLength) continue;

        if (isLeaf(nodeIdx)) {
            // Leaf node - intersect all primitives, early out on hit
            int start = getPrimStart(nodeIdx);
            int count = getPrimCount(nodeIdx);
            for (int i = 0; i < count; i++) {
                int primIdx = primIndices[start + i];
                if (intersect(ray, primitives[primIdx]) && ray.primitive.shaderType != 2) {
                    return true;
                }
            }
        } else {
            // Internal node
            int dim = getDimension(nodeIdx);
            float splitVal = getSplit(nodeIdx);
            float d = (splitVal - ray.origin[dim]) / ray.direction[dim];

            int front = ray.direction[dim] < 0.0 ? 1 : 0;
            int back = 1 - front;
            int childFront = (front == 0) ? getChildLeft(nodeIdx) : getChildRight(nodeIdx);
            int childBack = (back == 0) ? getChildLeft(nodeIdx) : getChildRight(nodeIdx);

            if (d <= t0 || d < 0.0) {
                if (childBack >= 0 && sp < MAX_STACK) {
                    stackNode[sp] = childBack;
                    stackT0[sp] = t0;
                    stackT1[sp] = t1;
                    sp++;
                }
            } else if (d >= t1) {
                if (childFront >= 0 && sp < MAX_STACK) {
                    stackNode[sp] = childFront;
                    stackT0[sp] = t0;
                    stackT1[sp] = t1;
                    sp++;
                }
            } else {
                if (childBack >= 0 && sp < MAX_STACK - 1) {
                    stackNode[sp] = childBack;
                    stackT0[sp] = d - SPLT_EPS;
                    stackT1[sp] = t1;
                    sp++;
                }
                if (childFront >= 0 && sp < MAX_STACK) {
                    stackNode[sp] = childFront;
                    stackT0[sp] = t0;
                    stackT1[sp] = d + SPLT_EPS;
                    sp++;
                }
            }
        }
    }

    return false;
}
