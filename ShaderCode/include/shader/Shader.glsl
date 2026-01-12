struct Shader {
    uint shaderType;
    int shaderIndex;
};

#include "FlatShader.glsl"
#include "MirrorShader.glsl"
#include "SimpleShadowShader.glsl"

layout(binding = 20, std430) buffer Shaders {
    uint shaderCount;
    Shader shaders[];
};

vec3 shade(inout Ray ray, inout vec3 throughput) {
    if (ray.primitive.shaderType == 1) {
        FlatShader flatShader = flatShaders[ray.primitive.shaderIndex];
        ray.remainingBounces = 0;
        return shadeFlat(ray, flatShader, throughput);
    } else if (ray.primitive.shaderType == 2) {
        MirrorShader mirrorShader = mirrorShaders[ray.primitive.shaderIndex];
        ray.remainingBounces--;
        return shadeMirror(ray, mirrorShader, throughput);
    } else if (ray.primitive.shaderType == 3) {
        SimpleShadowShader simpleShadowShader = simpleShadowShaders[ray.primitive.shaderIndex];
        ray.remainingBounces = 0;
        return shadeSimpleShadow(ray, simpleShadowShader, throughput);
    }
}
