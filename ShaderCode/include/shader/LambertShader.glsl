struct LambertShader {
    vec4 diffuseColor;
};

layout(binding = 25, std430) buffer LambertShaders {
    uint lambertShaderCount;
    LambertShader lambertShaders[];
};

vec3 shadeLambertShader(inout Ray ray, in LambertShader shader, in vec3 throughput) {
    const vec3 diffuseColor = shader.diffuseColor.xyz;
    ray.remainingBounces = 0;
    
    vec3 fragmentColor = vec3(0);

    for (int i = 0; i < lightCount; i++) {
        const Illumination illum = illuminate(ray, lights[i]);
        // Diffuse term
        const vec3 diffuse = diffuseColor * max(dot(-illum.direction, ray.normal), 0);
        fragmentColor += diffuse * illum.color;
    }
    return throughput * fragmentColor;
}