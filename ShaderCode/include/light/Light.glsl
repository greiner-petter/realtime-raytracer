struct Light {
    uint lightType;
    int lightIndex;
};

struct Illumination {
    vec3 color;
    vec3 direction;
};

#include "PointLight.glsl"

layout(binding = 30, std430) buffer Lights {
    uint lightCount;
    Light lights[];
};

Illumination illuminate(inout Hit hit, Light light) {
    if (light.lightType == 1) {
        PointLight pointLight = pointLights[light.lightIndex];
        return illuminatePointLight(hit, pointLight);
    }
}
