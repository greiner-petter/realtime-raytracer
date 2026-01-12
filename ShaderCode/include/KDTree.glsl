struct KDNode {
    int   left;        // index of left child (or -1)
    int   right;       // index of right child (or -1)

    int   axis;        // 0=x, 1=y, 2=z
    float split;       // split plane

    int   firstPrim;   // leaf only
    int   primCount;   // leaf only
};

layout(binding = 1, std430) buffer kdTree {
    vec4 minBounds; // x,y,z = min, w = padding
    vec4 maxBounds; // x,y,z = max, w = padding
    KDNode nodes[];
};

struct StackItem {
    int nodeIndex;
    float tMin;
    float tMax;
};

bool intersectAABB(Ray ray, vec3 minBounds, vec3 maxBounds, inout float t0, inout float t1) {
    float tmin = (minBounds.x - ray.origin.x) / ray.direction.x;
    float tmax = (maxBounds.x - ray.origin.x) / ray.direction.x;
    if (tmin > tmax) {
        float temp = tmax;
        tmax = tmin;
        tmin = temp;
    }

    float tymin = (minBounds.y - ray.origin.y) / ray.direction.y;
    float tymax = (maxBounds.y - ray.origin.y) / ray.direction.y;
    if (tymin > tymax) {
        float temp = tymax;
        tymax = tymin;
        tymin = temp;
    }

    if ((tmin > tymax) || (tymin > tmax)) return false;
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;

    float tzmin = (minBounds.z - ray.origin.z) / ray.direction.z;
    float tzmax = (maxBounds.z - ray.origin.z) / ray.direction.z;
    if (tzmin > tzmax) {
        float temp = tzmax;
        tzmax = tzmin;
        tzmin = temp;
    }

    if ((tmin > tzmax) || (tzmin > tmax)) return false;
    if (tzmin > tmin) tmin = tzmin;
    if (tzmax < tmax) tmax = tzmax;
    
    t0 = tmin;
    t1 = tmax;
    return true;
}

bool intersectKDTree(Ray ray, out Hit hit) {
    hit.rayLength = INFINITY;
    hit.primitiveIndex = -1;

    // 1. Initial Scene Intersection
    // We need the entry/exit points (tMin, tMax) for the global scene or root node.
    // Assuming 'nodes[0]' is the root and contains the full scene bounds.
    float rootTMin, rootTMax;
    if (!intersectAABB(ray, minBounds.xyz, maxBounds.xyz, rootTMin, rootTMax)) {
        return false;
    }

    // Stack setup
    StackItem stack[MAX_STACK];
    int sp = 0;
    
    // Push Root with full scene interval
    stack[sp].nodeIndex = 0;
    stack[sp].tMin = rootTMin;
    stack[sp].tMax = rootTMax;
    sp++;

    while (sp > 0) {
        // Pop node
        StackItem current = stack[--sp];
        
        // OPTIMIZATION: Occlusion Culling
        if (hit.rayLength < current.tMin) {
            continue;
        }
        
        // Load node data
        KDNode node = nodes[current.nodeIndex];

        // CASE: Leaf Node
        if (node.left < 0) { // Assuming negative index means leaf
            for (int i = node.firstPrim; i < node.firstPrim + node.primCount; ++i) {
                // intersectPrimitive should update 'hit' only if the new t is closer
                intersectPrimitive(ray, primitives[i]); 
            }
            continue;
        }

        // CASE: Internal Node
        int axis = node.axis;
        float t0 = current.tMin;
        float t1 = current.tMax;
        
        // Calculate distance to split plane
        float d = (node.split - ray.origin[axis]) / ray.direction[axis];

        // Determine Near (front) and Far (back) children relative to ray direction
        int frontChild = (ray.direction[axis] < 0.0) ? node.right : node.left;
        int backChild  = (ray.direction[axis] < 0.0) ? node.left  : node.right;

        // Apply the traversal logic similar to your findIntersection snippet.
        float d_near = d + SPLT_EPS;
        float d_far  = d - SPLT_EPS;

        if (d <= t0) {
            // Split is behind the interval start -> Traversed only Back child
            if (backChild != -1) {
                stack[sp].nodeIndex = backChild;
                stack[sp].tMin = t0;
                stack[sp].tMax = t1;
                sp++;
            }
        } 
        else if (d >= t1) {
            // Split is beyond the interval end -> Traversed only Front child
            if (frontChild != -1) {
                stack[sp].nodeIndex = frontChild;
                stack[sp].tMin = t0;
                stack[sp].tMax = t1;
                sp++;
            }
        } 
        else {
            // Split cuts the interval -> Traverse BOTH.

            // 1. Push Back (Far) Child: interval [d, t1]
            if (backChild != -1) {
                stack[sp].nodeIndex = backChild;
                stack[sp].tMin = d_near; // Start at split
                stack[sp].tMax = t1;
                sp++;
            }

            // 2. Push Front (Near) Child: interval [t0, d]
            if (frontChild != -1) {
                stack[sp].nodeIndex = frontChild;
                stack[sp].tMin = t0;
                stack[sp].tMax = d_far; // End at split
                sp++;
            }
        }
    }

    return hit.primitiveIndex != -1;
}