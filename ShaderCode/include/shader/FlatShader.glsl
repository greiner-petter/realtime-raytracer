struct FlatShader {
    vec4 objectColor; 
};

layout(binding = 21, std430) buffer FlatShaders {
    uint flatShaderCount;
    FlatShader flatShaders[];
};

vec3 shadeFlatShader(inout Ray ray, in FlatShader shader, in vec3 throughput) {
    const vec3 objectColor = shader.objectColor.xyz;
    ray.remainingBounces = 0;
    
    return throughput * objectColor;
}