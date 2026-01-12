struct RefractionShader {
    vec4 indexInside_Outside;
};

layout(binding = 22, std430) buffer RefractionShaders {
    uint refractionShaderCount;
    RefractionShader refractionShaders[];
};

vec3 shadeRefractionShader(inout Ray ray, in RefractionShader shader, inout vec3 throughput) {
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

    // Using the notation from the lecture
    float cosineTheta = dot(normalVector, -ray.direction);
    float cosinePhi = sqrt(1 + refractiveIndex * refractiveIndex * (cosineTheta * cosineTheta - 1)); // NaN if radicant is < 0
    // Calculate t, the new ray direction
    vec3 t = refractiveIndex * ray.direction + (refractiveIndex * cosineTheta - cosinePhi) * normalVector;

    // Create the refraction ray
    vec3 origin = vec3(0);
    vec3 direction = vec3(1);  

    // Check whether it is a refraction. NaN enters the else branch.
    if (dot(t, normalVector) <= 0.0) {
        origin = ray.origin + (ray.rayLength + REFR_EPS) * ray.direction;
        direction = normalize(t);
    } else { // Otherwise, it is a total reflection.
        origin = ray.origin + (ray.rayLength - REFR_EPS) * ray.direction;
        // Next we get the reflection vector
        const vec3 reflectionVector = reflect(ray.direction, normalVector);

        // Change the ray direction and origin
        direction = normalize(reflectionVector);
    }

    // Send out a new refracted ray into the scene
    Ray refractionRay = createRay(origin, direction, ray.remainingBounces);
    ray = refractionRay;
    return vec3(0);
}