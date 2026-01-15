#include "Sphere.glsl"
#include "Triangle.glsl"
#include "InfinitePlane.glsl"
#include "Box.glsl"

#include "Primitive.h.glsl"

bool intersect(inout Ray ray, in Primitive primitive) {
    const int i = primitive.primitiveIndex;
    if (primitive.primitiveType == 1) {
        if (intersectSphere(ray, spheres[i])) {
            ray.primitive = primitive;
            return true;
        }
    } else if (primitive.primitiveType == 2) {
        if (intersectTriangle(ray, triangles[i])) {
            ray.primitive = primitive;
            return true;
        }
    } else if (primitive.primitiveType == 3) {
        if (intersectInfinitePlane(ray, infinitePlanes[i])) {
            ray.primitive = primitive;
            return true;
        }
    } else if (primitive.primitiveType == 4) {
        if (intersectBox(ray, boxes[i])) {
            ray.primitive = primitive;
            return true;
        }
    }
    return false;
}
