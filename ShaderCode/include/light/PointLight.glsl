#include "Light.h.glsl"

struct PointLight {
    vec4 color_intensity;
    vec4 position_radius;
};

layout(binding = 31, std430) buffer PointLights {
    uint pointLightCount;
    PointLight pointLights[];
};

float rand();
vec3 traceTransmission(Ray shadowRay);

vec3 getPointLightPosition(vec4 position_radius) {
    vec3 randomDir = normalize(vec3(rand() - 0.5, rand() - 0.5, rand() - 0.5));
    return position_radius.xyz + randomDir * rand() * position_radius.w;
}

Illumination illuminatePointLight(inout Ray ray, in PointLight pointLight) {
    const vec3 target = ray.origin + (ray.rayLength - LGT_EPS) * ray.direction;

    vec3 pos = getPointLightPosition(pointLight.position_radius);

    // Illumination object
    Illumination illum = createIllumination(target - pos);

    // Precompute the distance from the light source
    const float dist = length(target - pos);

    // Define a secondary ray from the surface point to the light source.
    Ray lightRay = createRay(target, -illum.direction, 0);
    lightRay.rayLength = dist - LGT_EPS;

    // If the target is not in shadow...
    vec3 transmission = traceTransmission(lightRay);
    // ... compute the attenuation and light color
    illum.color = transmission * (1.0f / (dist * dist)) * pointLight.color_intensity.xyz * pointLight.color_intensity.w;
    
    return illum;
}