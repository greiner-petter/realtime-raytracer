#include "primitive/Primitive.h.glsl"
#include "light/Light.h.glsl"

bool intersect(inout Ray ray, in Primitive primitive);
vec3 shade(inout Ray ray, inout vec3 throughput, inout uint rngState);

// Forward declarations for KD-tree functions (defined in KDTree.glsl)
bool intersectKDTree(inout Ray ray);
bool occludeKDTree(inout Ray ray);

bool intersectScene(inout Ray ray) {
    return intersectKDTree(ray);
    bool didHit = false;
    for (int i = 0; i < primitiveCount; ++i) {
        if (intersect(ray, primitives[i]))
            didHit = true;
    }
    return didHit;
}

bool occludeScene(inout Ray ray) {
    return occludeKDTree(ray);
    for (int i = 0; i < primitiveCount; ++i) {
        if (intersect(ray, primitives[i]))
            return true;
    }
    return false;
}

vec3 getSkyColor(vec3 direction) {
    if (direction.y > 0.0) {
        return mix(vec3(0.9, 0.9, 1.0), vec3(0.5, 0.7, 1.0), direction.y);
    } else {
        return mix(vec3(0.9, 0.9, 1.0), vec3(1.0), -direction.y);
    }
}

vec3 traceRay(inout Ray ray, inout uint rngState) {    
    vec3 radiance = vec3(0);
    vec3 throughput = vec3(1);

    while (ray.remainingBounces > 0) {
        if (!intersectScene(ray)) {
            return radiance + throughput * getSkyColor(ray.direction);
        }
        radiance += throughput * shade(ray, throughput, rngState) / lightCount;
    }
    return radiance;
}