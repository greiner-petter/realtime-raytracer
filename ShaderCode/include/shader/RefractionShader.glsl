struct RefractionShader {
    vec4 indexInside_Outside;
};

layout(binding = 22, std430) buffer RefractionShaders {
    uint refractionShaderCount;
    RefractionShader refractionShaders[];
};

vec3 shadeRefractionShader(inout Ray ray, inout vec3 throughput) {
    const RefractionShader shader = refractionShaders[ray.primitive.shaderIndex];
    const float indexInside = shader.indexInside_Outside.x;
    const float indexOutside = shader.indexInside_Outside.y;

    // Get the normal of the primitive which was hit
    vec3 normalVector = ray.normal;

    // Calculate the index of refraction
    float refractiveIndex = indexOutside / indexInside;
    // What if we are already inside the object?
    // Note: This assumes normals pointing outwards, this is not the case for e.g. the Box primitive
    if (dot(normalVector, ray.direction) > 0) {
        normalVector = -normalVector;
        refractiveIndex = indexInside / indexOutside;
    }

    // Use built-in refract function (returns zero vector on total internal reflection)
    const vec3 refraction = refract(ray.direction, normalVector, refractiveIndex);

    // Create the refraction ray
    vec3 origin = vec3(0);
    vec3 direction = vec3(1);

    // Check whether it is a refraction (refract returns zero on TIR)
    if (refraction != vec3(0)) {
        origin = ray.origin + (ray.rayLength + REFR_EPS) * ray.direction;
        direction = refraction;
    } else {
        origin = ray.origin + (ray.rayLength - REFR_EPS) * ray.direction;
        direction = reflect(ray.direction, normalVector);
        ray.remainingBounces--;
    }

    // Send out a new refracted ray into the scene
    ray = createRay(origin, direction, ray.remainingBounces);
    return vec3(0);
}