
struct MaterialShader {
    vec4 alphaMap_opacity;
    vec4 normalMap_coefficient;
    vec4 diffuseMap_coefficient;
    vec4 reflectionMap_reflectance;
    vec4 specularMap_coefficient_exponent;
};

layout(binding = 30, std430) buffer MaterialShaders {
    uint materialShaderCount;
    MaterialShader materialShaders[];
};

vec3 tangentToWorldSpace(const vec3 surfaceNormal, const vec3 surfaceTangent, const vec3 surfaceBitangent, const vec3  textureNormal) {
  return textureNormal.x * surfaceTangent + textureNormal.y * surfaceBitangent + textureNormal.z * surfaceNormal;
}

vec3 shadeMaterialShaderGI(inout Ray ray, inout vec3 throughput) {
    // TODO: implement GI version
    ray.remainingBounces = 0;
    return vec3(0);
}

vec3 shadeMaterialShader(inout Ray ray, inout vec3 throughput) {
    MaterialShader shader = materialShaders[ray.primitive.shaderIndex];
    const int alphaMap = int(shader.alphaMap_opacity.x);
    const int normalMap = int(shader.normalMap_coefficient.x);
    const int diffuseMap = int(shader.diffuseMap_coefficient.x);
    const int reflectionMap = int(shader.reflectionMap_reflectance.x);
    const int specularMap = int(shader.specularMap_coefficient_exponent.x);
    const float opacity = shader.alphaMap_opacity.y;
    const float normalCoefficient = shader.normalMap_coefficient.y;
    const float diffuseCoefficient = shader.diffuseMap_coefficient.y;
    float reflectance = shader.reflectionMap_reflectance.y;
    const float specularCoefficient = shader.specularMap_coefficient_exponent.y;
    const float shininessExponent = shader.specularMap_coefficient_exponent.z;

    // Calculate alpha early for Russian roulette
    float alpha = opacity;
    if (alphaMap != -1)
        alpha *= sampleTex(alphaMap, ray.surface).r;

    // Calculate effective reflectance
    if (reflectionMap != -1)
        reflectance *= sampleTex(reflectionMap, ray.surface).r;

    // Weights for Russian roulette path selection
    float transparentWeight = 1.0 - alpha;
    float reflectWeight = alpha * reflectance;
    float diffuseWeight = alpha * (1.0 - reflectance);

    // Russian roulette: randomly select which path to follow
    float totalWeight = transparentWeight + reflectWeight + diffuseWeight;
    float random = rand() * totalWeight;

    if (random < transparentWeight) {
        // Pass through (alpha transparency) - no direct contribution
        vec3 origin = ray.origin + (ray.rayLength + EPSILON) * ray.direction;
        ray = createRay(origin, ray.direction, ray.remainingBounces - 1);
        // throughput stays the same (we're just passing through)
        return vec3(0);
    }

    // From here on, we hit opaque surface - compute normal and lighting

    // (Normal Map) Calculate the new normal vector
    vec3 normal = ray.normal;
    if (normalMap != -1) {
        const vec3 normalColor = sampleTex(normalMap, ray.surface).rgb;
        const vec3 textureNormal = vec3(2 * normalColor.r, 2 * normalColor.g, 2 * normalColor.b) - vec3(1);
        normal = normalize(tangentToWorldSpace(normal, ray.tangent, ray.bitangent, normalize(textureNormal)) * normalCoefficient + (1 - normalCoefficient) * normal);
    }

    // Calculate the reflection vector
    const vec3 reflection = reflect(ray.direction, normal);

    // Get diffuse color from texture
    vec3 diffuseColor = vec3(1.0);
    if (diffuseMap != -1)
        diffuseColor = sampleTex(diffuseMap, ray.surface).rgb;

    // Get specular color from texture
    vec3 specularColor = vec3(1.0);
    if (specularMap != -1)
        specularColor = sampleTex(specularMap, ray.surface).rgb;

    vec3 fragmentColor = vec3(0);

    // Compute direct lighting (diffuse + specular) from all lights
    for (int i = 0; i < lightCount; i++) {
        Illumination illum = illuminate(ray, lights[i]);
        float cosine = max(dot(-illum.direction, normal), 0.0);

        // Diffuse term
        fragmentColor += diffuseCoefficient * diffuseColor * illum.color * cosine;

        // Specular term
        float specCosine = dot(-illum.direction, reflection);
        if (specCosine > 0.0) {
            fragmentColor += specularCoefficient * specularColor * illum.color * pow(specCosine, shininessExponent);
        }
    }

    // Decide continuation: reflection or terminate
    if (random < transparentWeight + reflectWeight) {
        // Reflection path
        vec3 origin = ray.origin + (ray.rayLength - EPSILON) * ray.direction;
        ray = createRay(origin, reflection, ray.remainingBounces - 1);
        // Scale throughput by reflectance
        throughput *= reflectance;
        // Return direct lighting scaled by non-reflective portion
        return fragmentColor * (1.0 - reflectance);
    } else {
        // Diffuse path - no more bounces in non-GI mode
        ray.remainingBounces = 0;
        return fragmentColor;
    }
}
