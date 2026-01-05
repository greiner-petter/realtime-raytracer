#version 450
#extension GL_GOOGLE_include_directive : enable

#include "Constants.glsl"
#include "UBO.glsl"
#include "Ray.glsl"
#include "primitive/Primitive.glsl"
#include "KDTree.glsl"

bool TraceRay(Ray ray, out Hit hit) {
    hit.rayLength = INFINITY;
    hit.primitiveIndex = -1;
    for (int i = 0; i < primitiveCount; ++i) {
        intersectPrimitive(ray, i, hit); 
    }
    return hit.primitiveIndex != -1;
}

vec3 main_frag(vec2 ndc) {

    // Primary ray
    Ray ray = createRay(ndc, u_CameraPosition, u_CameraForward, u_CameraRight, u_CameraUp, u_FocusDistance);
    
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for (int bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
        Hit hit;
        if (!TraceKDTree(ray, hit)) {
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
layout(binding = 255, rgba8) uniform image2D resultImage;

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

    // Write Output over multiple samples
    // Format must match the image layout (rgba8 -> vec4)
    vec4 currentColor = imageLoad(resultImage, pixelCoords) * min(u_SampleIndex, 1.0);
    vec4 to_write = (vec4(pixelColor, 1.0) + currentColor * (u_SampleIndex)) / (u_SampleIndex + 1.0);
    imageStore(resultImage, pixelCoords, to_write);
}
