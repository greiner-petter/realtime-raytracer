struct PointLight {
    vec4 position;
    vec4 color_intensity;
};

layout(binding = 31, std430) buffer PointLights {
    uint pointLightCount;
    PointLight pointLights[];
};

Illumination illuminatePointLight(inout Hit hit, PointLight pointLight) {
    vec3 target = hit.point;

    // Illumination object
    Illumination illum;
    illum.color = vec3(0);
    illum.direction = normalize(target - pointLight.position.xyz);

    // Precompute the distance from the light source
    float dist = length(target - pointLight.position.xyz);

    // Define a secondary ray from the surface point to the light source.
    Ray lightRay;
    lightRay.origin = vec3(0);//target + hit.normal * 10 * EPSILON;
    lightRay.direction = -illum.direction;

    Hit lightHit;
    lightHit.rayLength = dist;

    // If the target is not in shadow...
    // if (lightHit.rayLength > dist - EPSILON) {
    if (!IntersectScene(lightRay, lightHit)) {
        // ... compute the attenuation and light color
        illum.color = 1.0 / (dist * dist) * pointLight.color_intensity.xyz * pointLight.color_intensity.w;
        illum.color = vec3(1,0,0);
    } else {
        illum.color = vec3(0,1,0);
    }
    return illum;
}