struct PhongShader {
    vec4 diffuseColor_Coefficient;
    vec4 specularColor_Coefficient;
    vec4 shininessExponent;
};

layout(binding = 26, std430) buffer PhongShaders {
    uint phongShaderCount;
    PhongShader phongShaders[];
};

Ray shadeIndirectLight(in Ray ray, in vec3 diffuseColor, inout vec3 throughput);
Light getRandomLight();

vec3 shadePhongShaderGI(inout Ray ray, inout vec3 throughput) {
    const PhongShader shader = phongShaders[ray.primitive.shaderIndex];
    const vec3 diffuseColor = shader.diffuseColor_Coefficient.xyz;
    const float diffuseCoefficient = shader.diffuseColor_Coefficient.w;
    const vec3 specularColor = shader.specularColor_Coefficient.xyz;
    const float specularCoefficient = shader.specularColor_Coefficient.w;
    const float shininessExponent = shader.shininessExponent.x;
    
    vec3 fragmentColor = vec3(0);

    // Calculate the reflection vector
    const vec3 reflection = reflect(ray.direction, ray.normal);

    const Illumination illum = illuminate(ray, getRandomLight());

    // Diffuse term
    const vec3 diffuse = diffuseCoefficient * diffuseColor * max(dot(-illum.direction, ray.normal), 0.0f);
    fragmentColor += diffuse * illum.color * lightCount;

    // Specular term
    const float cosine = dot(-illum.direction, reflection);
    if (cosine > 0) {
        const vec3 specular = specularCoefficient * specularColor // highlight
                                * pow(cosine, shininessExponent); // shininess factor
        fragmentColor += specular * illum.color * lightCount;
    }

    ray = shadeIndirectLight(ray, diffuseColor, throughput);

    return fragmentColor;
}

vec3 shadePhongShader(inout Ray ray, in vec3 throughput) {
    const PhongShader shader = phongShaders[ray.primitive.shaderIndex];
    const vec3 diffuseColor = shader.diffuseColor_Coefficient.xyz;
    const float diffuseCoefficient = shader.diffuseColor_Coefficient.w;
    const vec3 specularColor = shader.specularColor_Coefficient.xyz;
    const float specularCoefficient = shader.specularColor_Coefficient.w;
    const float shininessExponent = shader.shininessExponent.x;
    ray.remainingBounces = 0;
    
    vec3 fragmentColor = vec3(0);

    // Calculate the reflection vector
    const vec3 reflection = reflect(ray.direction, ray.normal);

    // Accumulate the light over all light sources
    for (int i = 0; i < lightCount; i++) {
        const Illumination illum = illuminate(ray, lights[i]);

        // Diffuse term
        const vec3 diffuse = diffuseCoefficient * diffuseColor * max(dot(-illum.direction, ray.normal), 0.0f);
        fragmentColor += diffuse * illum.color;

        // Specular term
        const float cosine = dot(-illum.direction, reflection);
        if (cosine > 0) {
            const vec3 specular = specularCoefficient * specularColor // highlight
                                    * pow(cosine, shininessExponent); // shininess factor
            fragmentColor += specular * illum.color;
        }
    }

    return fragmentColor;
}