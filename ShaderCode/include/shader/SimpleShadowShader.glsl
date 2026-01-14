struct SimpleShadowShader {
    vec4 objectColor;
};

layout(binding = 24, std430) buffer SimpleShadowShaders {
    uint simpleShadowShaderCount;
    SimpleShadowShader simpleShadowShaders[];
};

vec3 shadeSimpleShadowShader(inout Ray ray, in SimpleShadowShader shader, in vec3 throughput) {
    const vec3 objectColor = shader.objectColor.xyz;
    ray.remainingBounces = 0;
    
    vec3 fragmentColor = vec3(0);

    for (int i = 0; i < lightCount; i++) {
        fragmentColor += illuminate(ray, lights[i]).color;
    }
    return throughput * fragmentColor * objectColor;
}