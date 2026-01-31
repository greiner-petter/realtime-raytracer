struct MirrorShader {
    vec4 throughput_roughness;
};

layout(binding = 23, std430) buffer MirrorShaders {
    uint mirrorShaderCount;
    MirrorShader mirrorShaders[];
};

float rand();

vec3 shadeMirrorShader(inout Ray ray, inout vec3 throughput) {
    const MirrorShader shader = mirrorShaders[ray.primitive.shaderIndex];
    const vec3 through = shader.throughput_roughness.xyz;
    const float roughness = shader.throughput_roughness.w;

    const vec3 origin = ray.origin + (ray.rayLength - REFR_EPS) * ray.direction;
    const vec3 direction = reflect(ray.direction, ray.normal) + normalize(vec3(rand() - 0.5, rand() - 0.5, rand() - 0.5)) * roughness;

    // Create a new reflection ray
    ray = createRay(origin, direction, ray.remainingBounces - 1);

    throughput *= through;
    return vec3(0);
}