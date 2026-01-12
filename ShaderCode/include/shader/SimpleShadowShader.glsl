struct SimpleShadowShader {
    vec4 objectColor;
};

layout(binding = 23, std430) buffer SimpleShadowShaders {
    uint simpleShadowShaderCount;
    SimpleShadowShader simpleShadowShaders[];
};

vec3 shadeSimpleShadow(inout Ray ray, in SimpleShadowShader shader, in vec3 throughput) {
    vec3 fragmentColor = vec3(0);

    for (int i = 0; i < lightCount; i++) {
        fragmentColor += illuminate(ray, lights[i]).color;
    }
    return throughput * fragmentColor * shader.objectColor.xyz;
}