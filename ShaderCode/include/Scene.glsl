#include "primitive/Primitive.h.glsl"
#include "light/Light.h.glsl"

bool intersect(inout Ray ray, in Primitive primitive);
vec3 getGlassTransmission(in Ray ray);
vec3 shade(inout Ray ray, inout vec3 throughput);
vec3 shadeGI(inout Ray ray, inout vec3 throughput);
vec4 sampleTex(int ID, vec2 uv);

// Forward declarations for KD-tree functions (defined in KDTree.glsl)
bool intersectKDTree(inout Ray ray);
vec3 traceTransmissionKDTree(Ray ray);

bool intersectScene(inout Ray ray) {
    // return intersectKDTree(ray);
    bool didHit = false;
    for (int i = 0; i < primitiveCount; ++i) {
        if (intersect(ray, primitives[i]))
            didHit = true;
    }
    return didHit;
}

vec3 traceTransmission(Ray shadowRay) {
    // vec3 traceTransmissionKDTree(Ray ray);
    vec3 transmission = vec3(1);
    const vec3 startOrigin = shadowRay.origin;
    const float maxDist = shadowRay.rayLength;

    for (int bounce = 0; bounce < int(u_RayBounces); bounce++) {
        bool didHit = false;
        for (int i = 0; i < primitiveCount; ++i) {
            if (intersect(shadowRay, primitives[i])) {
                didHit = true;
            }
        }

        if (!didHit) {
            // Ray reached the light without further obstruction
            break;
        }

        // Get transmission (returns vec3(0) for opaque, positive for glass)
        vec3 glassTransmission = getGlassTransmission(shadowRay);
        if (glassTransmission == vec3(0)) {
            // Hit opaque object - fully blocked
            return vec3(0);
        }
        transmission *= glassTransmission;

        // Continue ray from the other side of the glass
        shadowRay.origin = shadowRay.origin + (shadowRay.rayLength + REFR_EPS * 2) * shadowRay.direction;
        shadowRay.rayLength = maxDist - length(shadowRay.origin - startOrigin);

        // Early exit if transmission is too low or we've traveled past the light
        if (shadowRay.rayLength <= 0 || max(transmission.r, max(transmission.g, transmission.b)) < 0.001) {
            break;
        }
    }

    return transmission;
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
            return radiance + throughput * sampleTex(int(u_EnvMapTexture), vec2(rho / (2.0f * float(PI)), phi / float(PI))).xyz;
        } else {
            // ... if all else fails, just return the background color
            return radiance + throughput * getSkyColor(ray.direction);
        }
    }
    return radiance;
}