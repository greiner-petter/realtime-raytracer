#include "Sphere.glsl"
#include "Triangle.glsl"
#include "InfinitePlane.glsl"

struct Primitive {
    uint type;
    int index;
    int materialID;
};

layout(binding = 2, std430) buffer Primitives {
    uint primitiveCount;
    Primitive primitives[];
};

void intersectPrimitive(Ray ray, int primIndex, inout Hit hit) {
    if (primitives[primIndex].type == 1) {
        Sphere sphere = spheres[primitives[primIndex].index];
        if (intersectSphere(ray, sphere, hit)) {
            hit.materialID = primitives[primIndex].materialID;
            hit.primitiveIndex = primIndex;
        }
    } else if (primitives[primIndex].type == 2) {
        Triangle triangle = triangles[primitives[primIndex].index];
        if (intersectTriangle(ray, triangle, hit)) {
            hit.materialID = primitives[primIndex].materialID;
            hit.primitiveIndex = primIndex;
        }
    } else if (primitives[primIndex].type == 3) {
        InfinitePlane plane = infinitePlanes[primitives[primIndex].index];
        if (intersectInfinitePlane(ray, plane, hit)) {
            hit.materialID = primitives[primIndex].materialID;
            hit.primitiveIndex = primIndex;
        }
    }
}
