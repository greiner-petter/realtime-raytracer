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

void intersectPrimitive(Ray ray, Primitive primitive, inout Hit hit) {
    if (primitive.type == 1) {
        Sphere sphere = spheres[primitive.index];
        if (intersectSphere(ray, sphere, hit)) {
            hit.materialID = primitive.materialID;
            hit.primitiveIndex = primitive.index;
        }
    } else if (primitive.type == 2) {
        Triangle triangle = triangles[primitive.index];
        if (intersectTriangle(ray, triangle, hit)) {
            hit.materialID = primitive.materialID;
            hit.primitiveIndex = primitive.index;
        }
    } else if (primitive.type == 3) {
        InfinitePlane plane = infinitePlanes[primitive.index];
        if (intersectInfinitePlane(ray, plane, hit)) {
            hit.materialID = primitive.materialID;
            hit.primitiveIndex = primitive.index;
        }
    }
}
