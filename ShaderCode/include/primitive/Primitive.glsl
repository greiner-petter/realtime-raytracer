struct Primitive {
    uint type;
    int index;
    int materialID;
};

layout(binding = 2, std430) buffer Primitives {
    uint primitiveCount;
    Primitive primitives[];
};

void intersectPrimitive(Ray ray, Primitive prim, int primIndex, inout Hit hit) {
    if (prim.type == 1) {
        Sphere sphere = spheres[prim.index];
        if (intersectSphere(ray, sphere, hit)) {
            hit.materialID = prim.materialID;
            hit.primitiveIndex = primIndex;
        }
    } else if (prim.type == 2) {
        Triangle triangle = triangles[prim.index];
        if (intersectTriangle(ray, triangle, hit)) {
            hit.materialID = prim.materialID;
            hit.primitiveIndex = primIndex;
        }
    } else if (prim.type == 3) {
        InfinitePlane plane = infinitePlanes[prim.index];
        if (intersectInfinitePlane(ray, plane, hit)) {
            hit.materialID = prim.materialID;
            hit.primitiveIndex = primIndex;
        }
    }
}
