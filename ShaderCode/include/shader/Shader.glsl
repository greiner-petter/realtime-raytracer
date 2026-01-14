struct Shader {
    uint shaderType;
    int shaderIndex;
};

#include "FlatShader.glsl"
#include "RefractionShader.glsl"
#include "MirrorShader.glsl"
#include "SimpleShadowShader.glsl"
#include "LambertShader.glsl"

layout(binding = 20, std430) buffer Shaders {
    uint shaderCount;
    Shader shaders[];
};

vec3 shade(inout Ray ray, inout vec3 throughput) {
    if (ray.primitive.shaderType == 1) {
        FlatShader flatShader = flatShaders[ray.primitive.shaderIndex];
        return shadeFlatShader(ray, flatShader, throughput);
    } else if (ray.primitive.shaderType == 2) {
        RefractionShader refractionShader = refractionShaders[ray.primitive.shaderIndex];
        return shadeRefractionShader(ray, refractionShader, throughput);
    } else if (ray.primitive.shaderType == 3) {
        MirrorShader mirrorShader = mirrorShaders[ray.primitive.shaderIndex];
        return shadeMirrorShader(ray, mirrorShader, throughput);
    } else if (ray.primitive.shaderType == 4) {
        SimpleShadowShader simpleShadowShader = simpleShadowShaders[ray.primitive.shaderIndex];
        return shadeSimpleShadowShader(ray, simpleShadowShader, throughput);
    } else if (ray.primitive.shaderType == 5) {
        LambertShader lambertShader = lambertShaders[ray.primitive.shaderIndex];
        return shadeLambertShader(ray, lambertShader, throughput);
    }
    return vec3(0);
}
