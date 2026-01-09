#include "FlatShader.glsl"
#include "MirrorShader.glsl"

struct Shader {
    uint shaderType;
    int shaderIndex;
};

layout(binding = 20, std430) buffer Shaders {
    uint shaderCount;
    Shader shaders[];
};

bool shade(inout Ray ray, in Hit hit, inout vec3 throughput, inout vec3 radiance) {
    if (hit.shaderType == 1) {
        FlatShader flatShader = flatShaders[hit.shaderIndex];
        radiance += throughput * flatShader.color.xyz;
    } else if (hit.shaderType == 2) {
        MirrorShader mirrorShader = mirrorShaders[hit.shaderIndex]; 
        ray.origin = hit.point + hit.normal * EPSILON;
        ray.direction = reflect(ray.direction, hit.normal);
        throughput *= mirrorShader.color.xyz;
        return true;
    }
    return false;
}
