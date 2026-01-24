struct SimpleShadowShader {
    vec4 objectColor;
};

layout(binding = 24, std430) buffer SimpleShadowShaders {
    uint simpleShadowShaderCount;
    SimpleShadowShader simpleShadowShaders[];
};

Light getRandomLight();
Ray shadeIndirectLight(in Ray ray, in vec3 diffuseColor, inout vec3 throughput);

vec3 shadeSimpleShadowShaderGI(inout Ray ray, in vec3 throughput) {
    const SimpleShadowShader shader = simpleShadowShaders[ray.primitive.shaderIndex];
    const vec3 objectColor = shader.objectColor.xyz;
    
    vec3 fragmentColor = illuminate(ray, getRandomLight()).color * lightCount;

    ray = shadeIndirectLight(ray, objectColor, throughput);
    
    return fragmentColor * objectColor;
}

vec3 shadeSimpleShadowShader(inout Ray ray, in vec3 throughput) {
    const SimpleShadowShader shader = simpleShadowShaders[ray.primitive.shaderIndex];
    const vec3 objectColor = shader.objectColor.xyz;
    ray.remainingBounces = 0;
    
    vec3 fragmentColor = vec3(0);
    for (int i = 0; i < lightCount; i++) {
        fragmentColor += illuminate(ray, lights[i]).color;
    }
    
    return fragmentColor * objectColor;
}