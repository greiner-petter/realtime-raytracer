
bool intersectSphere(Ray ray, Sphere sphere, inout Hit hit) {
    // Ray-sphere difference vector
    vec3 difference = ray.origin - sphere.center_radius.xyz;

    // Quadratic coefficients (a = 1 because direction is normalized)
    float a = 1.0;
    float b = 2.0 * dot(ray.direction, difference);
    float c = dot(difference, difference) - sphere.center_radius.w * sphere.center_radius.w;

    float discriminant = b * b - 4.0 * a * c;

    // No intersection
    if (discriminant < 0.0)
        return false;

    float root = sqrt(discriminant);

    // Numerically stable quadratic solution
    float q = -0.5 * (b < 0.0 ? (b - root) : (b + root));
    float t0 = q / a;
    float t1 = c / q;

    float t = min(t0, t1);
    if (t < EPSILON)
        t = max(t0, t1);

    // Reject if behind camera or farther than previous hit
    if (t < EPSILON || t > hit.rayLength)
        return false;

    // Compute hit point
    vec3 hitPoint = ray.origin + t * ray.direction;

    // Surface normal
    hit.normal = normalize(hitPoint - sphere.center_radius.xyz);

    // Spherical UV coordinates
    float phi = acos(hit.normal.y);
    float rho = atan(hit.normal.z, hit.normal.x) + PI;
    hit.surface = vec2(
        rho / (2.0 * PI),
        phi / PI
    );

    // Tangent space
    hit.tangent = vec3(sin(rho), 0.0, cos(rho));
    hit.bitangent = normalize(cross(hit.normal, hit.tangent));

    // Update ray hit distance
    hit.rayLength = t;
    hit.point = hitPoint;
    return true;
}
