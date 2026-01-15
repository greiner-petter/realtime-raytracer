#include "PointLight.glsl"

#include "Light.h.glsl"

Illumination illuminate(inout Ray ray, in Light light) {
    switch (light.lightType) {
        case 1: return illuminatePointLight(ray, pointLights[light.lightIndex]);
    }
}
