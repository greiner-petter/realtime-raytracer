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