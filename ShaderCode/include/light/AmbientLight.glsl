#include "Light.h.glsl"

struct AmbientLight {
    vec4 color_intensity;
};

layout(binding = 42, std430) buffer AmbientLights {
    uint ambientLightCount;
    AmbientLight ambientLights[];
};

Illumination illuminateAmbientLight(inout Ray ray, in AmbientLight ambientLight) {
    Illumination illum = createIllumination(-ray.normal);
    illum.color = (u_EnableGI > 0) ? vec3(0) : ambientLight.color_intensity.xyz * ambientLight.color_intensity.w;
    return illum;
}
