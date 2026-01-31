struct BrdfShader {
    vec4 scaleIndex;  // xyz = color scale, w = BRDF ID
};

layout(binding = 29, std430) buffer BrdfShaders {
    uint brdfShaderCount;
    BrdfShader brdfShaders[];
};

Ray shadeIndirectLight(in Ray ray, in vec3 diffuseColor, inout vec3 throughput);
Light getRandomLight();
vec3 lookup_brdf_values(float theta_in, float phi_in, float theta_out, float phi_out, int baseOffset);

vec3 shadeBRDFShaderGI(inout Ray ray, inout vec3 throughput) {
    BrdfShader shader = brdfShaders[ray.primitive.shaderIndex];
    vec3 scale = shader.scaleIndex.xyz;
    int brdfId = int(shader.scaleIndex.w);

    // Calculate theta_in (angle between view direction and normal)
    float theta_in = acos(dot(-ray.normal, ray.direction));
    float phi_in = 0;

    // Derive local coordinate system
    vec3 x = cross(-ray.direction, ray.normal);
    vec3 y = cross(ray.normal, x);

    vec3 illuminationColor = vec3(0);
    Illumination illum = illuminate(ray, getRandomLight());

    // Diffuse term
    float cosine = dot(-illum.direction, ray.normal);
    if (cosine > 0) {
        vec3 color = vec3(0);

        // Avoid numeric instability
        if (cosine < 1) {
            float theta_out = acos(cosine);

            // Project outgoing vector into local coordinate system
            vec3 c = cross(-illum.direction, ray.normal);
            float phi_out = 2 * atan(dot(c, y), dot(c, x));

            color = lookup_brdf_values(theta_in, phi_in, theta_out, phi_out, brdfId);
        } else {
            color = lookup_brdf_values(theta_in, phi_in, 0, 0, brdfId);
        }

        // Calculate colors
        vec3 diffuseColor = scale * color * cosine * lightCount;
        illuminationColor += diffuseColor * illum.color;

        ray = shadeIndirectLight(ray, scale, throughput);
    } else {
        ray.remainingBounces = 0;
    }

    return illuminationColor;
}

vec3 shadeBRDFShader(inout Ray ray, inout vec3 throughput) {
    BrdfShader shader = brdfShaders[ray.primitive.shaderIndex];
    vec3 scale = shader.scaleIndex.xyz;
    int brdfId = int(shader.scaleIndex.w);
    ray.remainingBounces = 0;

    // Calculate theta_in (angle between view direction and normal)
    float theta_in = acos(dot(-ray.normal, ray.direction));
    float phi_in = 0;

    // Derive local coordinate system
    vec3 x = cross(-ray.direction, ray.normal);
    vec3 y = cross(ray.normal, x);

    vec3 illuminationColor = vec3(0);
    // Accumulate the light over all light sources
    for (int i = 0; i < lightCount; i++) {
        Illumination illum = illuminate(ray, lights[i]);

        // Diffuse term
        float cosine = dot(-illum.direction, ray.normal);
        if (cosine > 0) {
            vec3 color = vec3(0);

            // Avoid numeric instability
            if (cosine < 1) {
                float theta_out = acos(cosine);

                // Project outgoing vector into local coordinate system
                vec3 c = cross(-illum.direction, ray.normal);
                float phi_out = 2 * atan(dot(c, y), dot(c, x));

                color = lookup_brdf_values(theta_in, phi_in, theta_out, phi_out, brdfId);
            } else {
                color = lookup_brdf_values(theta_in, phi_in, 0, 0, brdfId);
            }

            // Calculate colors
            vec3 diffuseColor = scale * color * cosine;
            illuminationColor += diffuseColor * illum.color;
        }
    }

    return illuminationColor;
}
