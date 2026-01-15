struct Sphere {
    vec4 center_radius; // xyz = center, w = radius
};

layout(binding = 11, std430) buffer Spheres {
    uint sphereCount;
    Sphere spheres[];
};

bool intersectSphere(inout Ray ray, in Primitive primitive) {
    const Sphere sphere = spheres[primitive.primitiveIndex];
    // Use the definitions from the lecture
    const vec3 difference = ray.origin - sphere.center_radius.xyz;
    const float a = 1.0f;
    const float b = 2.0f * dot(ray.direction, difference);
    const float c = dot(difference, difference) - sphere.center_radius.w * sphere.center_radius.w;
    const float discriminant = b * b - 4 * a * c;

    // Test whether the ray could intersect at all
    if (discriminant < 0)
        return false;
    const float root = sqrt(discriminant);

    // Stable solution
    const float q = -0.5f * (b < 0 ? (b - root) : (b + root));
    const float t0 = q / a;
    const float t1 = c / q;
    float t = min(t0, t1);
    if (t < EPSILON)
        t = max(t0, t1);

    // Test whether this is the foremost primitive in front of the camera
    if (t < EPSILON || ray.rayLength < t)
        return false;

    // Calculate the normal
    const vec3 hitPoint = ray.origin + t * ray.direction;
    ray.normal = normalize(hitPoint - sphere.center_radius.xyz);

    // Calculate the surface position and tangent vector
    const float phi = acos(ray.normal.y);
    const float rho = 2 * atan(ray.normal.z, ray.normal.x) + PI;
    ray.surface = vec2(rho / (2 * PI), phi / PI);
    ray.tangent = vec3(sin(rho), 0, cos(rho));
    ray.bitangent = normalize(cross(ray.normal, ray.tangent));

    // Set the new length and the current primitive
    ray.rayLength = t;
    ray.primitive = primitive;

    // True, because the primitive was hit
    return true;
}
