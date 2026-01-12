#include "primitive/Primitive.h.glsl"

struct Ray {
    vec3 origin;
    vec3 direction;
    float rayLength;
    Primitive primitive;
    vec3 normal;
    vec2 surface;
    vec3 tangent;
    vec3 bitangent;
    int remainingBounces;
};

Ray createRay(vec3 origin, vec3 direction, int remainingBounces) {
    Ray ray;
    ray.origin = origin;
    ray.direction = normalize(direction);
    ray.rayLength = INFINITY;
    ray.primitive = NULLPRIMITIVE;
    ray.remainingBounces = remainingBounces;
    return ray;
}
