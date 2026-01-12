#include "PointLight.glsl"

#include "Light.h.glsl"

Illumination illuminate(inout Ray ray, in Light light) {
    if (light.lightType == 1) {
        PointLight pointLight = pointLights[light.lightIndex];
        return illuminatePointLight(ray, pointLight);
    }
}
