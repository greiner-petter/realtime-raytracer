struct InfinitePlane {
    vec4 origin;
    vec4 normal;
};

layout(binding = 5, std430) buffer InfinitePlanes {
    uint InfinitePlaneCount;
    InfinitePlane infinitePlanes[];
};

bool intersectInfinitePlane(Ray ray, InfinitePlane plane, inout Hit hit) {
    float cosine = dot(ray.direction, plane.normal.xyz);

    // Make sure the ray is not coming from the other side (backface culling).
    // Note: We only use backface culling for InfinitePlanes, because we have
    // some special features planned that rely on backfaces for other primitives.
    if (cosine > 0)
        return false;

    // Determine the distance at which the ray intersects the plane
    float t = dot(plane.origin.xyz - ray.origin, plane.normal.xyz) / cosine;

    // Test whether this is the foremost primitive in front of the camera
    if (t < EPSILON || hit.rayLength < t)
        return false;

    // Set the normal
    hit.normal = plane.normal.xyz;

    // Set the new length and the current primitive
    hit.rayLength = t;
    hit.point = ray.origin + ray.direction * t;

    // True, because the primitive was hit
    return true;
}
