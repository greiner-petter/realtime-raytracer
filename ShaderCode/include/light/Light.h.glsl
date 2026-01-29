#ifndef LIGHT_H
#define LIGHT_H

struct Illumination {
    vec3 color;
    vec3 direction;
};

struct Light {
    uint lightType;
    int lightIndex;
};

layout(binding = 40, std430) buffer Lights {
    uint lightCount;
    Light lights[];
};

float rand();

Illumination createIllumination(in vec3 direction) {
    Illumination illum;
    illum.color = vec3(0);
    illum.direction = normalize(direction);
    return illum;
}

Light getRandomLight() {
    float random = rand();
    const int randomLightIndex = int(min(random * float(lightCount), float(lightCount) - 1.0));
    return lights[randomLightIndex];
}

#endif