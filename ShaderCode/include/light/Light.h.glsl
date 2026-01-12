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

layout(binding = 30, std430) buffer Lights {
    uint lightCount;
    Light lights[];
};

Illumination createIllumination(in vec3 direction) {
    Illumination illum;
    illum.color = vec3(0);
    illum.direction = normalize(direction);
    return illum;
}

#endif