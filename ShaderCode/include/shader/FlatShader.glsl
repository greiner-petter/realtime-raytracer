struct FlatShader {
    vec4 objectColor; 
};

layout(binding = 21, std430) buffer FlatShaders {
    uint flatShaderCount;
    FlatShader flatShaders[];
};

bool shadeFlat(in FlatShader shader, in vec3 throughput, inout vec3 radiance) {
    radiance = throughput * shader.objectColor.xyz;
    return false;
}