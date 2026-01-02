#version 450
layout(location = 0) out vec4 outColor;

layout(location = 1) in vec2 v_UV;

layout(binding = 0) uniform UBO {
    vec2 u_resolution;
    float u_aspectRatio;
    float u_FocusDistance;
    vec3 u_CameraPosition;
    vec3 u_CameraForward;
    vec3 u_CameraRight;
    vec3 u_CameraUp;
};

//include "Constants.glsl"
//include "Ray.glsl"

struct Primitive {
    uint type;
    int index;
    int materialID;
};

struct Sphere {
    vec4 center_radius; // xyz = center, w = radius
};

layout(binding = 1, std430) buffer Primitives {
    uint primitiveCount;
    Primitive primitives[];
};

layout(binding = 2, std430) buffer Spheres {
    uint sphereCount;
    Sphere spheres[];
};

//include "intersect/Sphere.glsl"

bool TraceRay(Ray ray, out Hit hit) {
    hit.rayLength = INFINITY;
    hit.primitiveIndex = -1;
    bool found = false;
    for (uint i = 0; i < primitiveCount; ++i) {
        if (primitives[int(i)].type == 1) {
            Sphere sphere = spheres[primitives[int(i)].index];
            if (intersectSphere(ray, sphere, hit)) {
                found = true;
                hit.materialID = primitives[int(i)].materialID;
                hit.primitiveIndex = int(i);
            }
        }
    }
    return found;
}

Ray createRay(vec2 ndc, vec3 cameraPosition, vec3 cameraForward, vec3 cameraRight, vec3 cameraUp, float focus) {
    Ray ray;
    ray.origin = cameraPosition;

    ray.direction = normalize(
        ndc.x * cameraRight +
        ndc.y * cameraUp +
        focus * cameraForward
    );

    return ray;
}

void main() {
    vec2 ndc = v_UV;
    ndc.y *= -1.0;
    ndc.y *= u_aspectRatio;

    // Primary ray
    Ray ray = createRay(ndc, u_CameraPosition, u_CameraForward, u_CameraRight, u_CameraUp, u_FocusDistance);
    
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for (int bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
        Hit hit;
        if (!TraceRay(ray, hit)) {
            // sky
            vec3 skyColor = vec3(0.0);
            if (ray.direction.y > 0.0) {
                skyColor = mix(vec3(0.9, 0.9, 1.0), vec3(0.5, 0.7, 1.0), ray.direction.y);
            } else {
                skyColor = mix(vec3(0.9, 0.9, 1.0), vec3(1.0), -ray.direction.y);
            }
            radiance += throughput * skyColor;
            break;
        }

        // Unlit / emissive
        if (hit.materialID == 0) {
            radiance += throughput * vec3(1, 0.2, 0);
            break;
        }

        // Mirror
        if (hit.materialID == 1) {
            ray.origin = hit.point + hit.normal * EPSILON;
            ray.direction = reflect(ray.direction, hit.normal);
            throughput *= vec3(0.8);
            continue;
        }

        // No material matched, terminate
        break;
    }

    outColor = vec4(radiance, 1.0);
}