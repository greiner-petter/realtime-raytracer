#include "FlatShader.glsl"
#include "RefractionShader.glsl"
#include "MirrorShader.glsl"
#include "SimpleShadowShader.glsl"
#include "LambertShader.glsl"
#include "PhongShader.glsl"
#include "SimpleTextureShader.glsl"

struct Shader {
    uint shaderType;
    int shaderIndex;
};

layout(binding = 20, std430) buffer Shaders {
    uint shaderCount;
    Shader shaders[];
};

float rand();

vec3 uniformSampleHemisphere(float r1, float r2) {
    float sinTheta = sqrt(1.0 - r1 * r1);
    float phi = 2.0 * PI * r2;
    float x = sinTheta * cos(phi);
    float z = sinTheta * sin(phi);
    return vec3(x, r1, z);
}

Ray shadeIndirectLight(in Ray ray, in vec3 diffuseColor, inout vec3 throughput) {
    float r1 = rand(); float r2 = rand();
    const vec3 sampleLocal = uniformSampleHemisphere(r1, r2);

    const vec3 normal = ray.normal;
    const float sign = (normal.z >= 0.0) ? 1.0 : -1.0;
    const vec3 tangent = vec3(1.0 + sign * normal.x * normal.x * -1.0 / (sign + normal.z), sign * normal.x * normal.y * -1.0 / (sign + normal.z), -sign * normal.x);
    const vec3 bitangent = vec3(normal.x * normal.y * -1.0 / (sign + normal.z), sign + normal.y * normal.y * -1.0 / (sign + normal.z), -normal.y);

    const vec3 sampleWorld = normalize(sampleLocal.x * bitangent + sampleLocal.y * normal + sampleLocal.z * tangent);

    const vec3 hitpoint = ray.origin + (ray.rayLength - EPSILON) * ray.direction;
    Ray indirectRay = createRay(hitpoint, sampleWorld, ray.remainingBounces - 1);

    float cosTheta = max(dot(normal, sampleWorld), 0.0);
    throughput *= diffuseColor * cosTheta * 2.0;

    return indirectRay;
}

vec3 shade(inout Ray ray, inout vec3 throughput) {
    switch (ray.primitive.shaderType) {
        case 1: return shadeFlatShader(ray, throughput);
        case 2: return shadeRefractionShader(ray, throughput);
        case 3: return shadeMirrorShader(ray, throughput);
        case 4: return shadeSimpleShadowShader(ray, throughput);
        case 5: return shadeLambertShader(ray, throughput);
        case 6: return shadePhongShader(ray, throughput);
        case 10: return shadeSimpleTextureShader(ray, throughput);
    }
    return vec3(0);
}

vec3 shadeGI(inout Ray ray, inout vec3 throughput) {
    switch (ray.primitive.shaderType) {
        case 1: return shadeFlatShader(ray, throughput);
        case 2: return shadeRefractionShader(ray, throughput);
        case 3: return shadeMirrorShader(ray, throughput);
        case 4: return shadeSimpleShadowShaderGI(ray, throughput);
        case 5: return shadeLambertShaderGI(ray, throughput);
        // case 6: return shadePhongShaderGI(ray, throughput);
        // case 10: return shadeSimpleTextureShaderGI(ray, throughput);
    }
    return vec3(0);
}
