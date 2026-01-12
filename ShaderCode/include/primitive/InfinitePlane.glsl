struct InfinitePlane {
    vec4 origin;
    vec4 normal;
};

layout(binding = 13, std430) buffer InfinitePlanes {
    uint InfinitePlaneCount;
    InfinitePlane infinitePlanes[];
};

bool intersectInfinitePlane(inout Ray ray, in InfinitePlane plane) {
    const float cosine = dot(ray.direction, plane.normal.xyz);

    // Make sure the ray is not coming from the other side (backface culling).
    // Note: We only use backface culling for InfinitePlanes, because we have
    // some special features planned that rely on backfaces for other primitives.
    if (cosine > 0)
        return false;

    // Determine the distance at which the ray intersects the plane
    const float t = dot(plane.origin.xyz - ray.origin, plane.normal.xyz) / cosine;

    // Test whether this is the foremost primitive in front of the camera
    if (t < EPSILON || ray.rayLength < t)
        return false;

    // Set the normal
    ray.normal = plane.normal.xyz;

    // Set the new length and the current primitive
    ray.rayLength = t;

    // True, because the primitive was hit
    return true;
}
