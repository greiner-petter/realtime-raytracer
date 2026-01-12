struct FlatShader {
    vec4 objectColor; 
};

layout(binding = 21, std430) buffer FlatShaders {
    uint flatShaderCount;
    FlatShader flatShaders[];
};

vec3 shadeFlat(inout Ray ray, in FlatShader shader, in vec3 throughput) {
    return throughput * shader.objectColor.xyz;
}