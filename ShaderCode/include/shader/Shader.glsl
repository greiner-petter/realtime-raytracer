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

bool shade(inout Ray ray, inout vec3 throughput, inout vec3 radiance) {
    if (ray.primitive.shaderType == 1) {
        FlatShader flatShader = flatShaders[ray.primitive.shaderIndex];
        return shadeFlat(flatShader, throughput, radiance);
    } else if (ray.primitive.shaderType == 2) {
        MirrorShader mirrorShader = mirrorShaders[ray.primitive.shaderIndex]; 
        return shadeMirror(ray, mirrorShader, throughput);
    } else if (ray.primitive.shaderType == 3) {
        SimpleShadowShader simpleShadowShader = simpleShadowShaders[ray.primitive.shaderIndex];
        return shadeSimpleShadow(ray, simpleShadowShader, throughput, radiance);
    }
    return false;
}
