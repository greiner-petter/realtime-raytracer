#version 450
#extension GL_GOOGLE_include_directive : enable

#include "Constants.glsl"
#include "UBO.glsl"
#include "Textures.glsl"
#include "Ray.glsl"
#include "primitive/Primitive.glsl"
#include "KDTree.glsl"
#include "Scene.glsl"
#include "light/Light.glsl"
#include "shader/Shader.glsl"


layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(binding = 255, rgba32f) uniform image2D resultImage;

// Global random state
uint g_rngState;
uint g_rngCounter;

uint hash(uint x) {
    x ^= x >> 16;
    x *= 0x85ebca6bu;
    x ^= x >> 13;
    x *= 0xc2b2ae35u;
    x ^= x >> 16;
    return x;
}

float rand() {
    g_rngCounter++;
    return float(hash(g_rngState + g_rngCounter)) / 4294967295.0;
}

void initRng(uvec2 pixel, uint sampleIndex) {
    g_rngState = hash(pixel.x + hash(pixel.y + hash(sampleIndex)));
    g_rngCounter = 0u;
}

vec3 clamp(vec3 c) {
  return vec3(max(0.0f, min(c.x, 1.0f)), max(0.0f, min(c.y, 1.0f)), max(0.0f, min(c.z, 1.0f)));
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 screenDims = imageSize(resultImage);

    // Because workgroups are fixed size (e.g., 16x16), the total threads might 
    // slightly exceed the image size. We must return early to avoid writing out of bounds.
    if (pixelCoords.x >= screenDims.x || pixelCoords.y >= screenDims.y) {
        return;
    }

    // Initialize global RNG
    initRng(uvec2(pixelCoords), u_SampleIndex);

    // center the ray in the pixel.
    vec2 antiAliasJitter = vec2(rand(), rand()) - 0.5;
    antiAliasJitter *= 0.5; // reduce jitter amount
    vec2 uv = (vec2(pixelCoords) + vec2(0.5) + antiAliasJitter) / vec2(screenDims);

    // conversion from [0,1] UV to [-1,1] Clip Space for ray direction
    vec2 ndc = uv * 2.0 - 1.0; 
    
    // Correct for Aspect Ratio
    ndc.y *= -1.0;
    ndc.y *= u_aspectRatio;

    vec3 origin = u_CameraPosition;
    vec3 direction = normalize(
        ndc.x * u_CameraRight +
        ndc.y * u_CameraUp +
        u_FocusDistance * u_CameraForward
    );

    Ray ray = createRay(origin, direction, MAX_BOUNCES);

    // Raytrace
    vec3 pixelColor = traceRay(ray);

    // Write Output over multiple samples
    // Format must match the image layout (rgba8 -> vec4)
    vec4 prev = imageLoad(resultImage, pixelCoords);
    float n = float(u_SampleIndex);
    vec4 outColor = (prev * n + vec4(pixelColor, 1.0)) / (n + 1.0);
    imageStore(resultImage, pixelCoords, outColor);
}
