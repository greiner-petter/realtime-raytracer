#include "Sphere.glsl"
#include "Triangle.glsl"
#include "InfinitePlane.glsl"
#include "Box.glsl"

#include "Primitive.h.glsl"

bool intersectPrimitive(inout Ray ray, in Primitive primitive) {
    if (primitive.primitiveType == 1) {
        Sphere sphere = spheres[primitive.primitiveIndex];
        if (intersectSphere(ray, sphere)) {
            ray.primitive = primitive;
            return true;
        }
    } else if (primitive.primitiveType == 2) {
        Triangle triangle = triangles[primitive.primitiveIndex];
        if (intersectTriangle(ray, triangle)) {
            ray.primitive = primitive;
            return true;
        }
    } else if (primitive.primitiveType == 3) {
        InfinitePlane plane = infinitePlanes[primitive.primitiveIndex];
        if (intersectInfinitePlane(ray, plane)) {
            ray.primitive = primitive;
            return true;
        }
    } else if (primitive.primitiveType == 4) {
        Box box = boxes[primitive.primitiveIndex];
        if (intersectBox(ray, box)) {
            ray.primitive = primitive;
            return true;
        }
    }
    return false;
}
