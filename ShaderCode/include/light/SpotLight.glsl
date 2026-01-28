#include "Light.h.glsl"

struct SpotLight {
    vec4 color_intensity;
    vec4 position;
    vec4 direction;
    vec4 alphaMin_Max;
};

layout(binding = 33, std430) buffer SpotLights {
    uint spotLightCount;
    SpotLight spotLights[];
};

vec3 traceTransmission(Ray shadowRay);

Illumination illuminateSpotLight(inout Ray ray, in SpotLight spotLight) {
    const vec3 target = ray.origin + (ray.rayLength - LGT_EPS) * ray.direction;

    // Illumination object
    Illumination illum = createIllumination(target - spotLight.position.xyz);

    // Precompute the distance from the light source
    const float dist = length(target - spotLight.position.xyz);

    // Define a secondary ray from the surface point to the light source
    Ray lightRay = createRay(target, -illum.direction, 0);
    lightRay.rayLength = dist - LGT_EPS;

    // Determine the angle of the inner cone
    const float alpha = abs(acos(dot(illum.direction, spotLight.direction.xyz)) * 180.0f / float(PI));

    // If the target is within the cone...
    if (spotLight.alphaMin_Max.y > alpha) {
        // ... and not in shadow ...
        vec3 transmission = traceTransmission(lightRay);
        // ... compute the attenuation and light color ...
        illum.color = transmission * (1.0f / (dist * dist)) * spotLight.color_intensity.xyz * spotLight.color_intensity.w;
        // ... then compute the falloff towards the edge of the cone
        if (spotLight.alphaMin_Max.x < alpha)
            illum.color *= 1.0f - (alpha - spotLight.alphaMin_Max.x) / (spotLight.alphaMin_Max.y - spotLight.alphaMin_Max.x);
    }
    return illum;
}
