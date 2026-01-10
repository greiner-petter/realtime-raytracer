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

vec3 GetSkyColor(vec3 direction) {
    if (direction.y > 0.0) {
        return mix(vec3(0.9, 0.9, 1.0), vec3(0.5, 0.7, 1.0), direction.y);
    } else {
        return mix(vec3(0.9, 0.9, 1.0), vec3(1.0), -direction.y);
    }
}

bool intersectScene(inout Ray ray);
bool shade(inout Ray ray, inout vec3 throughput, inout vec3 radiance);

vec3 TraceRay(inout Ray ray) {    
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for (int bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
        if (!intersectScene(ray)) {
            // sky
            radiance += throughput * GetSkyColor(ray.direction);
            break;
        }
        if (!shade(ray, throughput, radiance)) {
            // no bounce
            break;
        }
    }

    return radiance;
}