struct MirrorShader {
    vec4 throughput;
};

layout(binding = 22, std430) buffer MirrorShaders {
    uint mirrorShaderCount;
    MirrorShader mirrorShaders[];
};

bool shadeMirror(inout Ray ray, in Hit hit, in MirrorShader shader, inout vec3 throughput) {
    ray.origin = hit.point + hit.normal * EPSILON;
    ray.direction = reflect(ray.direction, hit.normal);
    throughput *= shader.throughput.xyz;
    return true;
}