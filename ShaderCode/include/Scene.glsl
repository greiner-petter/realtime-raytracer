#include "primitive/Primitive.h.glsl"
#include "light/Light.h.glsl"

bool intersect(inout Ray ray, in Primitive primitive);
bool isTransparent(in Ray ray);
vec3 shade(inout Ray ray, inout vec3 throughput);
vec3 shadeGI(inout Ray ray, inout vec3 throughput);
vec4 sampleTex(int ID, vec2 uv);

// Forward declarations for KD-tree functions (defined in KDTree.glsl)
bool intersectKDTree(inout Ray ray);
bool occludeKDTree(inout Ray ray);

bool intersectScene(inout Ray ray) {
    // return intersectKDTree(ray);
    bool didHit = false;
    for (int i = 0; i < primitiveCount; ++i) {
        if (intersect(ray, primitives[i]))
            didHit = true;
    }
    return didHit;
}

bool occludeScene(inout Ray ray) {
    // return occludeKDTree(ray);
    for (int i = 0; i < primitiveCount; ++i) {
        if (intersect(ray, primitives[i]) && !isTransparent(ray))
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
        if (intersectScene(ray)) {
            // If the ray has hit an object, call the shader ...
            vec3 currentThroughput = throughput;
            vec3 emission = (u_EnableGI > 0) ? shadeGI(ray, throughput) : shade(ray, throughput);
            radiance += currentThroughput * emission;
        } else if (u_EnvMapTexture != 0xFFFFFFFF) {
            // ... otherwise look up the environment map ...
            const float phi = acos(ray.direction.y);
            const float rho = 2 * atan(ray.direction.z, ray.direction.x) + float(PI);
            return sampleTex(int(u_EnvMapTexture), vec2(rho / (2.0f * float(PI)), phi / float(PI))).xyz;
        } else {
            // ... if all else fails, just return the background color
            return radiance + throughput * getSkyColor(ray.direction);
        }
    }
    return radiance;
}