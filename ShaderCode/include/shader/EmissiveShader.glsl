struct EmissiveShader {
    vec4 color_intensity;
};

layout(binding = 31, std430) buffer EmissiveShaders {
    uint emissiveShaderCount;
    EmissiveShader emissiveShaders[];
};

vec3 shadeEmissiveShader(inout Ray ray, inout vec3 throughput) {
    const EmissiveShader shader = emissiveShaders[ray.primitive.shaderIndex];
    const vec3 color = shader.color_intensity.xyz;
    const float intensity = shader.color_intensity.w;
    ray.remainingBounces = 0;

    return color * intensity;
}
