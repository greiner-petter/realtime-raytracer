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
    switch (ray.primitive.shaderType) {
        case 1: return shadeFlatShader(ray, throughput);
        case 2: return shadeRefractionShader(ray, throughput);
        case 3: return shadeMirrorShader(ray, throughput);
        case 4: return shadeSimpleShadowShader(ray, throughput);
        case 5: return shadeLambertShader(ray, throughput);
        case 6: return shadePhongShader(ray, throughput);
    }
    return vec3(0);
}
