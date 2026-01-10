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

bool shade(inout Ray ray, inout Hit hit, inout vec3 throughput, inout vec3 radiance) {
    if (hit.shaderType == 1) {
        FlatShader flatShader = flatShaders[hit.shaderIndex];
        return shadeFlat(flatShader, throughput, radiance);
    } else if (hit.shaderType == 2) {
        MirrorShader mirrorShader = mirrorShaders[hit.shaderIndex]; 
        return shadeMirror(ray, hit, mirrorShader, throughput);
    } else if (hit.shaderType == 3) {
        SimpleShadowShader simpleShadowShader = simpleShadowShaders[hit.shaderIndex];
        return shadeSimpleShadow(hit, simpleShadowShader, throughput, radiance);
    }
    return false;
}
