#include "primitive/Primitive.h.glsl"

bool intersectPrimitive(inout Ray ray, in Primitive primitive);
vec3 shade(inout Ray ray, inout vec3 throughput);

bool intersectScene(inout Ray ray) {
    bool didHit = false;
    for (int i = 0; i < primitiveCount; ++i) {
        if (intersectPrimitive(ray, primitives[i]))
            didHit = true;
    }
    return didHit;
}

bool occludeScene(inout Ray ray) {
    for (int i = 0; i < primitiveCount; ++i) {
        if (intersectPrimitive(ray, primitives[i]))
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

vec3 traceRay(inout Ray ray) {    
    vec3 radiance = vec3(0);
    vec3 throughput = vec3(1);

    while (ray.remainingBounces > 0) {
        if (!intersectScene(ray)) {
            return throughput * getSkyColor(ray.direction);
        }
        radiance += shade(ray, throughput);
    }
    return radiance;
}