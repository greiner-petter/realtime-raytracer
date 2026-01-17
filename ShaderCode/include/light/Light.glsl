#include "PointLight.glsl"
#include "AmbientLight.glsl"
#include "SpotLight.glsl"

#include "Light.h.glsl"

Illumination illuminate(inout Ray ray, in Light light) {
    switch (light.lightType) {
        case 1: return illuminatePointLight(ray, pointLights[light.lightIndex]);
        case 2: return illuminateAmbientLight(ray, ambientLights[light.lightIndex]);
        case 3: return illuminateSpotLight(ray, spotLights[light.lightIndex]);
    }
    return createIllumination(ray.normal);
}
