#include "Light.h.glsl"

struct PointLight {
    vec4 position;
    vec4 color_intensity;
};

layout(binding = 31, std430) buffer PointLights {
    uint pointLightCount;
    PointLight pointLights[];
};

bool occludeScene(inout Ray ray);

Illumination illuminatePointLight(inout Ray ray, in PointLight pointLight) {
    const vec3 target = ray.origin + (ray.rayLength - LGT_EPS) * ray.direction;

    // Illumination object
    Illumination illum = createIllumination(target - pointLight.position.xyz);

    // Precompute the distance from the light source
    const float dist = length(target - pointLight.position.xyz);

    // Define a secondary ray from the surface point to the light source.
    Ray lightRay = createRay(target, -illum.direction, 0);
    lightRay.rayLength = dist - LGT_EPS;

    // If the target is not in shadow...
    if (!occludeScene(lightRay)){
        // ... compute the attenuation and light color
        illum.color = 1.0f / (dist * dist) * pointLight.color_intensity.xyz * pointLight.color_intensity.w;
    }
    return illum;
}