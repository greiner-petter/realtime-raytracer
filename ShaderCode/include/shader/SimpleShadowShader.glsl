struct SimpleShadowShader {
    vec4 objectColor;
};

layout(binding = 23, std430) buffer SimpleShadowShaders {
    uint simpleShadowShaderCount;
    SimpleShadowShader simpleShadowShaders[];
};

bool shadeSimpleShadow(inout Hit hit, in SimpleShadowShader shader, inout vec3 throughput, inout vec3 radiance) {
    vec3 fragmentColor = vec3(0);

    for (int i = 0; i < lightCount; i++) {
        fragmentColor += illuminate(hit, lights[i]).color;
    }
    radiance += fragmentColor * vec3(1.0) * throughput;
    return false;
}