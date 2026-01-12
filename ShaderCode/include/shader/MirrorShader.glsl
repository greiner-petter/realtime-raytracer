struct MirrorShader {
    vec4 throughput;
};

layout(binding = 22, std430) buffer MirrorShaders {
    uint mirrorShaderCount;
    MirrorShader mirrorShaders[];
};

vec3 shadeMirror(inout Ray ray, in MirrorShader shader, inout vec3 throughput) {
    const vec3 reflectionOrigin = ray.origin + (ray.rayLength - REFR_EPS) * ray.direction;
    const vec3 reflectionDirection = reflect(ray.direction, ray.normal);

    // Create a new reflection ray
    Ray reflectionRay = createRay(reflectionOrigin, reflectionDirection, ray.remainingBounces);

    ray = reflectionRay;
    throughput *= shader.throughput.xyz;
    return vec3(0);
}