struct LambertShader {
    vec4 diffuseColor;
};

layout(binding = 25, std430) buffer LambertShaders {
    uint lambertShaderCount;
    LambertShader lambertShaders[];
};

float rand();
Ray shadeIndirectLight(in Ray ray, in vec3 diffuseColor, inout vec3 throughput);
Light getRandomLight();

vec3 shadeLambertShaderGI(inout Ray ray, inout vec3 throughput) {
    const LambertShader shader = lambertShaders[ray.primitive.shaderIndex];
    const vec3 diffuseColor = shader.diffuseColor.xyz;

    const Illumination illum = illuminate(ray, getRandomLight());
    // Diffuse term
    const vec3 diffuse = diffuseColor * max(dot(-illum.direction, ray.normal), 0);
    vec3 fragmentColor = diffuse * illum.color * lightCount;

    ray = shadeIndirectLight(ray, diffuseColor, throughput);

    return fragmentColor;
}

vec3 shadeLambertShader(inout Ray ray, inout vec3 throughput) {
    const LambertShader shader = lambertShaders[ray.primitive.shaderIndex];
    const vec3 diffuseColor = shader.diffuseColor.xyz;
    ray.remainingBounces = 0;

    vec3 fragmentColor = vec3(0);

    for (int i = 0; i < lightCount; i++) {
        const Illumination illum = illuminate(ray, lights[i]);
        // Diffuse term
        const vec3 diffuse = diffuseColor * max(dot(-illum.direction, ray.normal), 0);
        fragmentColor += diffuse * illum.color;
    }
    return fragmentColor;
}
