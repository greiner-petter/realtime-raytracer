struct MirrorShader {
    vec4 color;
};

layout(binding = 22, std430) buffer MirrorShaders {
    uint mirrorShaderCount;
    MirrorShader mirrorShaders[];
};