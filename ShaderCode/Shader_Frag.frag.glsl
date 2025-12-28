#version 450
layout(location = 0) out vec4 outColor;

layout(location = 1) in vec2 v_UV;

layout(binding = 0) uniform UBO {
    vec2 u_resolution;
    float u_aspectRatio;
    float u_FocusDistance;
    vec3 u_CameraPosition;
    vec3 u_CameraForward;
    vec3 u_CameraRight;
    vec3 u_CameraUp;
};

// ---------- Constants ----------
const int MAX_BOUNCES = 4;
const float EPSILON = 1e-4;
const float PI = 3.14159265359;


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
    int sphereIndex; // -1 if no hit
};

struct Sphere {
    vec4 center_radius; // xyz = center, w = radius
};

layout(binding = 1, std430) buffer Scene {
    uint sphereCount;
    Sphere spheres[];
};


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

bool TraceRay(Ray ray, out Hit hit) {
    hit.rayLength = 1e30;
    hit.sphereIndex = -1;
    bool found = false;
    for (uint i = 0; i < sphereCount; ++i) {
        Sphere sphere = spheres[i];
        if (intersectSphere(ray, sphere, hit)) {
            found = true;
            hit.materialID = 0; // Placeholder
            hit.sphereIndex = int(i);
        }
    }
    return found;
}

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

void main() {
    vec2 ndc = v_UV;
    ndc.y *= -1.0;
    ndc.y *= u_aspectRatio;

    // Primary ray
    Ray ray = createRay(ndc, u_CameraPosition, u_CameraForward, u_CameraRight, u_CameraUp, u_FocusDistance);
    
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for (int bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
        Hit hit;
        if (!TraceRay(ray, hit)) {
            // sky
            vec3 skyColor = vec3(0.0);
            if (ray.direction.y > 0.0) {
                skyColor = mix(vec3(0.9, 0.9, 1.0), vec3(0.5, 0.7, 1.0), ray.direction.y);
            } else {
                skyColor = mix(vec3(0.9, 0.9, 1.0), vec3(1.0), -ray.direction.y);
            }
            radiance += throughput * skyColor;
            break;
        }

        // Unlit / emissive
        if (hit.sphereIndex == 0) {
            radiance += throughput * vec3(1, 0.2, 0);
            break;
        }

        // Mirror
        if (hit.sphereIndex == 1) {
            ray.origin = hit.point + hit.normal * EPSILON;
            ray.direction = reflect(ray.direction, hit.normal);
            throughput *= vec3(0.8);
            continue;
        }

        // No material matched, terminate
        break;
    }

    outColor = vec4(radiance, 1.0);
}