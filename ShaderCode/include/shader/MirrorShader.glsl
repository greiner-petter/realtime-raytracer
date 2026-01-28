struct MirrorShader {
    vec4 through;
};

layout(binding = 23, std430) buffer MirrorShaders {
    uint mirrorShaderCount;
    MirrorShader mirrorShaders[];
};

vec3 shadeMirrorShader(inout Ray ray, inout vec3 throughput) {
    const MirrorShader shader = mirrorShaders[ray.primitive.shaderIndex];
    const vec3 through = shader.through.xyz;

    const vec3 origin = ray.origin + (ray.rayLength - REFR_EPS) * ray.direction;
    const vec3 direction = reflect(ray.direction, ray.normal);

    // Create a new reflection ray
    ray = createRay(origin, direction, ray.remainingBounces - 1);

    throughput *= through;
    return vec3(0);
}