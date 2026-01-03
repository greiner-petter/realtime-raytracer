#version 450

layout(binding = 0) uniform UBO {
    vec2 u_resolution;
    float u_aspectRatio;
    float u_FocusDistance;
    vec3 u_CameraPosition;
    vec3 u_CameraForward;
    vec3 u_CameraRight;
    vec3 u_CameraUp;
};

#include "Constants.glsl"
#include "Ray.glsl"

struct Primitive {
    uint type;
    int index;
    int materialID;
};

struct Sphere {
    vec4 center_radius; // xyz = center, w = radius
};

struct Triangle {
    vec4 vertex[3];
    vec4 normal[3];
    vec4 tangent[3];
    vec4 bitangent[3];
    vec4 surface[3];
};

layout(binding = 1, std430) buffer Primitives {
    uint primitiveCount;
    Primitive primitives[];
};

layout(binding = 2, std430) buffer Spheres {
    uint sphereCount;
    Sphere spheres[];
};

layout(binding = 3, std430) buffer Triangles {
    uint triangleCount;
    Triangle triangles[];
};

#include "intersect/Sphere.glsl"
#include "intersect/Triangle.glsl"

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
        } else if (primitives[int(i)].type == 2) {
            Triangle triangle = triangles[primitives[int(i)].index];
            if (intersectTriangle(ray, triangle, hit)) {
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

vec3 main_frag(vec2 ndc) {

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

    return radiance;
}


layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(binding = 255, rgba8) uniform writeonly image2D resultImage;


void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 screenDims = imageSize(resultImage);

    // Because workgroups are fixed size (e.g., 16x16), the total threads might 
    // slightly exceed the image size. We must return early to avoid writing out of bounds.
    if (pixelCoords.x >= screenDims.x || pixelCoords.y >= screenDims.y) {
        return;
    }

    // center the ray in the pixel.
    vec2 uv = (vec2(pixelCoords) + 0.5) / vec2(screenDims);

    // conversion from [0,1] UV to [-1,1] Clip Space for ray direction
    vec2 ndc = uv * 2.0 - 1.0; 
    
    // Correct for Aspect Ratio
    ndc.y *= -1.0;
    ndc.y *= u_aspectRatio;

    // Raytrace
    vec3 pixelColor = main_frag(ndc);

    // Write Output
    // Format must match the image layout (rgba8 -> vec4)
    imageStore(resultImage, pixelCoords, vec4(pixelColor, 1.0));
}
