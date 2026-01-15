#include "FlatShader.glsl"
#include "RefractionShader.glsl"
#include "MirrorShader.glsl"
#include "SimpleShadowShader.glsl"
#include "LambertShader.glsl"
#include "PhongShader.glsl"

struct Shader {
    uint shaderType;
    int shaderIndex;
};

layout(binding = 20, std430) buffer Shaders {
    uint shaderCount;
    Shader shaders[];
};

vec3 shade(inout Ray ray, inout vec3 throughput) {
    const int i = ray.primitive.shaderIndex;
    switch (ray.primitive.shaderType) {
        case 1: return shadeFlatShader(ray, flatShaders[i], throughput);
        case 2: return shadeRefractionShader(ray, refractionShaders[i], throughput);
        case 3: return shadeMirrorShader(ray, mirrorShaders[i], throughput);
        case 4: return shadeSimpleShadowShader(ray, simpleShadowShaders[i], throughput);
        case 5: return shadeLambertShader(ray, lambertShaders[i], throughput);
        case 6: return shadePhongShader(ray, phongShaders[i], throughput);
    }
    return vec3(0);
}
