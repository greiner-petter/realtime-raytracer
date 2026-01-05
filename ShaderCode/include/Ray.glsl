struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Hit {
    float rayLength;
    vec3 point;
    vec3 normal;
    vec2 surface;
    vec3 tangent;
    vec3 bitangent;
    int materialID;
    int primitiveIndex; // -1 if no hit
};

Ray createRay(vec2 ndc, vec3 cameraPosition, vec3 cameraForward, vec3 cameraRight, vec3 cameraUp, float focus) {
    Ray ray;
    ray.origin = cameraPosition;

    ray.direction = normalize(
        ndc.x * cameraRight +
        ndc.y * cameraUp +
        focus * cameraForward
    );

    return ray;
}