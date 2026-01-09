#include "Sphere.glsl"
#include "Triangle.glsl"
#include "InfinitePlane.glsl"

struct Primitive {
    uint primitiveType;
    int primitiveIndex;
    uint shaderType;
    int shaderIndex;
};

layout(binding = 10, std430) buffer Primitives {
    uint primitiveCount;
    Primitive primitives[];
};

void intersectPrimitive(Ray ray, Primitive primitive, inout Hit hit) {
    if (primitive.primitiveType == 1) {
        Sphere sphere = spheres[primitive.primitiveIndex];
        if (intersectSphere(ray, sphere, hit)) {
            hit.primitiveIndex = primitive.primitiveIndex;
            hit.shaderType = primitive.shaderType;
            hit.shaderIndex = primitive.shaderIndex;
        }
    } else if (primitive.primitiveType == 2) {
        Triangle triangle = triangles[primitive.primitiveIndex];
        if (intersectTriangle(ray, triangle, hit)) {
            hit.primitiveIndex = primitive.primitiveIndex;
            hit.shaderType = primitive.shaderType;
            hit.shaderIndex = primitive.shaderIndex;
        }
    } else if (primitive.primitiveType == 3) {
        InfinitePlane plane = infinitePlanes[primitive.primitiveIndex];
        if (intersectInfinitePlane(ray, plane, hit)) {
            hit.primitiveIndex = primitive.primitiveIndex;
            hit.shaderType = primitive.shaderType;
            hit.shaderIndex = primitive.shaderIndex;
        }
    }
}
