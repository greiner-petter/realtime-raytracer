struct LambertShader {
    vec4 diffuseColor;
};

layout(binding = 25, std430) buffer LambertShaders {
    uint lambertShaderCount;
    LambertShader lambertShaders[];
};

float rand(); // Global rand() defined in Raytracer.comp.glsl

vec3 uniformSampleHemisphere(float r1, float r2) {
    float sinTheta = sqrt(1.0 - r1 * r1);
    float phi = 2.0 * PI * r2;
    float x = sinTheta * cos(phi);
    float z = sinTheta * sin(phi);
    return vec3(x, r1, z);
}

vec3 shadeLambertShaderGI(inout Ray ray, inout vec3 throughput) {
    const LambertShader shader = lambertShaders[ray.primitive.shaderIndex];
    const vec3 diffuseColor = shader.diffuseColor.xyz;

    vec3 fragmentColor = vec3(0);

    float random = rand();
    const int randomLightIndex = int(min(random * float(lightCount), float(lightCount) - 1.0));
    const Light randomLight = lights[randomLightIndex];
    const Illumination illum = illuminate(ray, randomLight);
    // Diffuse term
    const vec3 diffuse = diffuseColor * max(dot(-illum.direction, ray.normal), 0);
    fragmentColor += diffuse * illum.color * lightCount;

    float r1 = rand(); float r2 = rand();
    const vec3 sampleLocal = uniformSampleHemisphere(r1, r2);

    const vec3 normal = ray.normal;
    const float sign = (normal.z >= 0.0) ? 1.0 : -1.0;
    const vec3 tangent = vec3(1.0 + sign * normal.x * normal.x * -1.0 / (sign + normal.z), sign * normal.x * normal.y * -1.0 / (sign + normal.z), -sign * normal.x);
    const vec3 bitangent = vec3(normal.x * normal.y * -1.0 / (sign + normal.z), sign + normal.y * normal.y * -1.0 / (sign + normal.z), -normal.y);

    const vec3 sampleWorld = normalize(sampleLocal.x * bitangent + sampleLocal.y * normal + sampleLocal.z * tangent);

    const vec3 hitpoint = ray.origin + (ray.rayLength - EPSILON) * ray.direction;
    Ray indirectRay = createRay(hitpoint, sampleWorld, ray.remainingBounces - 1);
    ray = indirectRay;

    float cosTheta = max(dot(normal, sampleWorld), 0.0);
    throughput *= diffuseColor * cosTheta * 2.0;

    return fragmentColor;
}

vec3 shadeLambertShader(inout Ray ray, inout vec3 throughput) {
    if (u_EnableGI > 0)
        return shadeLambertShaderGI(ray, throughput);

    const LambertShader shader = lambertShaders[ray.primitive.shaderIndex];
    const vec3 diffuseColor = shader.diffuseColor.xyz;
    ray.remainingBounces = 0;

    vec3 fragmentColor = vec3(0);

    for (int i = 0; i < lightCount; i++) {
        const Illumination illum = illuminate(ray, lights[i]);
        // Diffuse term
        const vec3 diffuse = diffuseColor * max(dot(-illum.direction, ray.normal), 0);
        fragmentColor += diffuse * illum.color;
    }
    return throughput * fragmentColor;
}
