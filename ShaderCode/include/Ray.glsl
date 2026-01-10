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
    int primitiveIndex; // -1 if no hit
    uint shaderType;
    int shaderIndex;
};

Ray createRay(vec3 origin, vec3 direction, int remainingBounces) {
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;
    return ray;
}

// Ray createRay(vec2 ndc, vec3 cameraPosition, vec3 cameraForward, vec3 cameraRight, vec3 cameraUp, float focus) {
//     Ray ray;
//     ray.origin = cameraPosition;

//     ray.direction = normalize(
//         ndc.x * cameraRight +
//         ndc.y * cameraUp +
//         focus * cameraForward
//     );

//     ray.rayLength = INFINITY;
//     ray.primitiveIndex = -1;

//     return ray;
// }

vec3 GetSkyColor(vec3 direction) {
    if (direction.y > 0.0) {
        return mix(vec3(0.9, 0.9, 1.0), vec3(0.5, 0.7, 1.0), direction.y);
    } else {
        return mix(vec3(0.9, 0.9, 1.0), vec3(1.0), -direction.y);
    }
}

bool IntersectScene(Ray ray, inout Hit hit);
bool IntersectKDTree(Ray ray, out Hit hit);
bool shade(inout Ray ray, inout Hit hit, inout vec3 throughput, inout vec3 radiance);

vec3 TraceRay(Ray ray) {    
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for (int bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
        Hit hit;
        hit.rayLength = INFINITY;
        if (!IntersectScene(ray, hit)) {
            // sky
            radiance += throughput * GetSkyColor(ray.direction);
            break;
        }
        if (!shade(ray, hit, throughput, radiance)) {
            // no bounce
            break;
        }
    }

    return radiance;
}