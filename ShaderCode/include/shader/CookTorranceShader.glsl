struct CookTorranceShader {
    vec4 diffuseColor_f0;
    vec4 ctColor_m;
};

layout(binding = 27, std430) buffer CookTorranceShaders {
    uint cookTorranceShaderCount;
    CookTorranceShader cookTorranceShaders[];
};

Ray shadeIndirectLight(in Ray ray, in vec3 diffuseColor, inout vec3 throughput);
Light getRandomLight();

float D(float NdotH, float m)  {
  // Beckmann distribution
  float r2 = m * m;
  float NdotH2 = NdotH * NdotH;
  return exp((NdotH2 - 1.0) / (r2 * NdotH2)) / (4.0 * r2 * pow(NdotH, 4.0));
}

float F(float VdotH, float F0) {
  // Schlicks approximation
  return F0 + (1.0f - F0) * pow(1.0f - VdotH, 5);
}

float G(float NdotH, float NdotV, float VdotH, float NdotL) {
    return min(1.0, min(2.0 * NdotH * NdotV / VdotH, 2.0 * NdotH * NdotL / VdotH));
}

vec3 shadeCookTorranceShaderGI(inout Ray ray, inout vec3 throughput) {
    const CookTorranceShader shader = cookTorranceShaders[ray.primitive.shaderIndex];
    const vec3 diffuseColor = shader.diffuseColor_f0.xyz;
    const vec3 ctColor = shader.ctColor_m.xyz;
    const float f0 = shader.diffuseColor_f0.w;
    const float m = shader.ctColor_m.w;

    vec3 fragmentColor = vec3(0);

    if (m >= 0) {
        Illumination illum = illuminate(ray, getRandomLight());

        const float NdotL = max(0, dot(-illum.direction, ray.normal));
        if (NdotL <= 0) {
            ray = shadeIndirectLight(ray, diffuseColor, throughput);
            return fragmentColor;
        }

        // Diffuse term
        const vec3 diffuse = diffuseColor / PI;
        fragmentColor += diffuse * NdotL * illum.color * lightCount;

        // Cook-Torrance term
        // half angle vector
        const vec3 H = normalize(-illum.direction - ray.direction);
        const float NdotH = max(0, dot(ray.normal, H));
        const float NdotV = max(0, dot(ray.normal, -ray.direction));
        const float VdotH = max(0, dot(-ray.direction, H));

        if (NdotV * NdotL > EPSILON) {
            const vec3 specular = ctColor * (F(VdotH, f0) * D(NdotH, m) * G(NdotH, NdotV, VdotH, NdotL)) / (PI * NdotV * NdotL);

            fragmentColor += specular * NdotL * illum.color * lightCount;
        }
        
        ray = shadeIndirectLight(ray, diffuseColor, throughput);
    } else {
        ray.remainingBounces = 0;
    }

    return fragmentColor;
}

vec3 shadeCookTorranceShader(inout Ray ray, in vec3 throughput) {
    const CookTorranceShader shader = cookTorranceShaders[ray.primitive.shaderIndex];
    const vec3 diffuseColor = shader.diffuseColor_f0.xyz;
    const vec3 ctColor = shader.ctColor_m.xyz;
    const float f0 = shader.diffuseColor_f0.w;
    const float m = shader.ctColor_m.w;
    ray.remainingBounces = 0;

    vec3 fragmentColor = vec3(0);

    if (m >= 0) {
        // Accumulate the light over all light sources
        for (int i = 0; i < lightCount; i++) {
            Illumination illum = illuminate(ray, lights[i]);

            const float NdotL = max(0, dot(-illum.direction, ray.normal));
            if (NdotL <= 0)
                continue;

            // Diffuse term
            const vec3 diffuse = diffuseColor / PI;
            fragmentColor += diffuse * NdotL * illum.color;

            // Cook-Torrance term
            // half angle vector
            const vec3 H = normalize(-illum.direction - ray.direction);
            const float NdotH = max(0, dot(ray.normal, H));
            const float NdotV = max(0, dot(ray.normal, -ray.direction));
            const float VdotH = max(0, dot(-ray.direction, H));

            if (NdotV * NdotL > EPSILON) {
                const vec3 specular = ctColor * (F(VdotH, f0) * D(NdotH, m) * G(NdotH, NdotV, VdotH, NdotL)) / (PI * NdotV * NdotL);

                fragmentColor += specular * NdotL * illum.color;
            }
        }
    }
    return fragmentColor;
}
