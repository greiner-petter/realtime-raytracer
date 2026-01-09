struct FlatShader {
    vec4 color; 
};

layout(binding = 21, std430) buffer FlatShaders {
    uint flatShaderCount;
    FlatShader flatShaders[];
};