struct SimpleTextureShader {
    vec4 color_texture; 
};

layout(binding = 27, std430) buffer SimpleTextureShaders {
    uint simpleTextureShaderCount;
    SimpleTextureShader simpleTextureShaders[];
};

vec3 shadeSimpleTextureShader(inout Ray ray, in vec3 throughput) {
    const SimpleTextureShader shader = simpleTextureShaders[ray.primitive.shaderIndex];
    const vec3 objectColor = shader.color_texture.xyz;
    int texture = int(shader.color_texture.w);
    ray.remainingBounces = 0;
    
    return throughput * objectColor * sampleTex(texture, ray.surface).rgb;
}