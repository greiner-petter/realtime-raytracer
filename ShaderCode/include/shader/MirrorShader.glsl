struct MirrorShader {
    vec4 throughput;
};

layout(binding = 23, std430) buffer MirrorShaders {
    uint mirrorShaderCount;
    MirrorShader mirrorShaders[];
};

vec3 shadeMirrorShader(inout Ray ray, inout vec3 throughput) {
    const MirrorShader shader = mirrorShaders[ray.primitive.shaderIndex];
    const vec3 thrpt = shader.throughput.xyz;
    const vec3 reflectionOrigin = ray.origin + (ray.rayLength - REFR_EPS) * ray.direction;
    const vec3 reflectionDirection = reflect(ray.direction, ray.normal);

    // Create a new reflection ray
    Ray reflectionRay = createRay(reflectionOrigin, reflectionDirection, ray.remainingBounces - 1);

    ray = reflectionRay;
    throughput *= thrpt;
    return vec3(0);
}