#version 450
layout(location = 0) out vec4 outColor;

layout(location = 1) in vec2 v_UV;

layout(binding = 0) uniform UBO {
    vec2 u_resolution;
    float u_aspectRatio;
};

const float EPSILON = 1e-4;
const float PI = 3.14159265359;


struct Ray {
    vec3 origin;
    vec3 direction;
    float rayLength;
    vec3 normal;
    vec2 surface;
    vec3 tangent;
    vec3 bitangent;
};

struct Sphere {
    vec4 center_radius; // xyz = center, w = radius
};

layout(binding = 1, std430) buffer Scene {
    uint sphereCount;
    Sphere spheres[];
};


bool intersectSphere(inout Ray ray, Sphere sphere) {
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
    if (t < EPSILON || t > ray.rayLength)
        return false;

    // Compute hit point
    vec3 hitPoint = ray.origin + t * ray.direction;

    // Surface normal
    ray.normal = normalize(hitPoint - sphere.center_radius.xyz);

    // Spherical UV coordinates
    float phi = acos(ray.normal.y);
    float rho = atan(ray.normal.z, ray.normal.x) + PI;

    ray.surface = vec2(
        rho / (2.0 * PI),
        phi / PI
    );

    // Tangent space
    ray.tangent = vec3(sin(rho), 0.0, cos(rho));
    ray.bitangent = normalize(cross(ray.normal, ray.tangent));

    // Update ray hit distance
    ray.rayLength = t;

    return true;
}

Ray createRay(vec2 ndc, vec3 cameraPosition, vec3 cameraForward, vec3 cameraRight, vec3 cameraUp, float focus)
{
    Ray ray;
    ray.origin = cameraPosition;

    ray.direction = normalize(
        ndc.x * cameraRight +
        ndc.y * cameraUp +
        focus * cameraForward
    );

    ray.rayLength = 1e30; // Large initial value

    return ray;
}

void main()
{
    vec2 ndc = v_UV;
    ndc.y *= -1.0;
    ndc.y *= u_aspectRatio;

    Ray ray = createRay(ndc, vec3(0, 0, 0), vec3(0, 0, -1), vec3(1, 0, 0), vec3(0, 1, 0), 1.0);
    
    bool hit = false;
    for (uint i = 0; i < sphereCount; ++i)
    {
        Sphere sphere = spheres[i];
        if (intersectSphere(ray, sphere)) {
            hit = true;
        }
    }


    if (hit)
    {
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
    else
    {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}