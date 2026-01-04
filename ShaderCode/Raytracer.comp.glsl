#version 450

layout(binding = 0) uniform UBO {
    vec2 u_resolution;
    float u_aspectRatio;
    float u_FocusDistance;
    vec3 u_CameraPosition;
    vec3 u_CameraForward;
    vec3 u_CameraRight;
    vec3 u_CameraUp;
};

#include "Constants.glsl"
#include "Ray.glsl"
#include "Primitives.glsl"

struct KDNode {
    int   left;        // index of left child (or -1)
    int   right;       // index of right child (or -1)

    int   axis;        // 0=x, 1=y, 2=z
    float split;       // split plane

    int   firstPrim;   // leaf only
    int   primCount;   // leaf only
};

layout(binding = 4, std430) buffer kdTree {
    vec4 minBounds; // x,y,z = min, w = padding
    vec4 maxBounds; // x,y,z = max, w = padding
    KDNode nodes[];
};


layout(binding = 1, std430) buffer Primitives {
    uint primitiveCount;
    Primitive primitives[];
};

layout(binding = 2, std430) buffer Spheres {
    uint sphereCount;
    Sphere spheres[];
};

layout(binding = 3, std430) buffer Triangles {
    uint triangleCount;
    Triangle triangles[];
};

#include "intersect/Sphere.glsl"
#include "intersect/Triangle.glsl"
#include "intersect/Primitive.glsl"

struct StackItem {
    int nodeIndex;
    float tMin;
    float tMax;
};

bool intersectAABB(Ray ray, vec3 minBounds, vec3 maxBounds, inout float t0, inout float t1) {
    float tmin = (minBounds.x - ray.origin.x) / ray.direction.x;
    float tmax = (maxBounds.x - ray.origin.x) / ray.direction.x;
    if (tmin > tmax) {
        float temp = tmax;
        tmax = tmin;
        tmin = temp;
    }

    float tymin = (minBounds.y - ray.origin.y) / ray.direction.y;
    float tymax = (maxBounds.y - ray.origin.y) / ray.direction.y;
    if (tymin > tymax) {
        float temp = tymax;
        tymax = tymin;
        tymin = temp;
    }

    if ((tmin > tymax) || (tymin > tmax)) return false;
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;

    float tzmin = (minBounds.z - ray.origin.z) / ray.direction.z;
    float tzmax = (maxBounds.z - ray.origin.z) / ray.direction.z;
    if (tzmin > tzmax) {
        float temp = tzmax;
        tzmax = tzmin;
        tzmin = temp;
    }

    if ((tmin > tzmax) || (tzmin > tmax)) return false;
    if (tzmin > tmin) tmin = tzmin;
    if (tzmax < tmax) tmax = tzmax;
    
    t0 = tmin;
    t1 = tmax;
    return true;
}

bool TraceRay(Ray ray, out Hit hit) {
    hit.rayLength = INFINITY;
    hit.primitiveIndex = -1;

    // 1. Initial Scene Intersection
    // We need the entry/exit points (tMin, tMax) for the global scene or root node.
    // Assuming 'nodes[0]' is the root and contains the full scene bounds.
    float rootTMin, rootTMax;
    if (!intersectAABB(ray, minBounds.xyz, maxBounds.xyz, rootTMin, rootTMax)) {
        return false;
    }

    // Stack setup
    StackItem stack[MAX_STACK];
    int sp = 0;
    
    // Push Root with full scene interval
    stack[sp].nodeIndex = 0;
    stack[sp].tMin = rootTMin;
    stack[sp].tMax = rootTMax;
    sp++;

    while (sp > 0) {
        // Pop node
        StackItem current = stack[--sp];
        
        // OPTIMIZATION: Occlusion Culling
        // If we found a hit in a previously visited 'Near' node that is closer 
        // than the start of this 'Far' node, we can skip it entirely.
        if (hit.rayLength < current.tMin) {
            continue;
        }
        
        // Load node data
        KDNode node = nodes[current.nodeIndex];

        // ---------------------------------------------------------
        // CASE: Leaf Node
        // ---------------------------------------------------------
        if (node.left < 0) { // Assuming negative index means leaf
            for (int i = node.firstPrim; i < node.firstPrim + node.primCount; ++i) {
                // intersectPrimitive should update 'hit' only if the new t is closer
                intersectPrimitive(ray, primitives[i], i, hit); 
            }
            continue;
        }

        // ---------------------------------------------------------
        // CASE: Internal Node
        // ---------------------------------------------------------
        int axis = node.axis;
        float t0 = current.tMin;
        float t1 = current.tMax;
        
        // Calculate distance to split plane
        // d = (split - rayOrigin) / rayDir
        float d = (node.split - ray.origin[axis]) / ray.direction[axis];

        // Determine Near (front) and Far (back) children relative to ray direction
        // If ray.direction is positive, Near is Left child, Far is Right child.
        // If ray.direction is negative, Near is Right child, Far is Left child.
        int frontChild = (ray.direction[axis] < 0.0) ? node.right : node.left;
        int backChild  = (ray.direction[axis] < 0.0) ? node.left  : node.right;

        // Apply the traversal logic similar to your findIntersection snippet.
        // We use SPLT_EPS to prevent cracks at the split plane.
        float d_near = d + SPLT_EPS;
        float d_far  = d - SPLT_EPS;

        if (d <= t0) {
            // Split is behind the interval start -> Traversed only Back child
            if (backChild != -1) {
                stack[sp].nodeIndex = backChild;
                stack[sp].tMin = t0;
                stack[sp].tMax = t1;
                sp++;
            }
        } 
        else if (d >= t1) {
            // Split is beyond the interval end -> Traversed only Front child
            if (frontChild != -1) {
                stack[sp].nodeIndex = frontChild;
                stack[sp].tMin = t0;
                stack[sp].tMax = t1;
                sp++;
            }
        } 
        else {
            // Split cuts the interval -> Traverse BOTH.
            // PUSH ORDER: We want to visit Front THEN Back.
            // Since it's a LIFO stack, we push BACK first, then FRONT.

            // 1. Push Back (Far) Child: interval [d, t1]
            if (backChild != -1) {
                stack[sp].nodeIndex = backChild;
                stack[sp].tMin = d_near; // Start at split
                stack[sp].tMax = t1;
                sp++;
            }

            // 2. Push Front (Near) Child: interval [t0, d]
            if (frontChild != -1) {
                stack[sp].nodeIndex = frontChild;
                stack[sp].tMin = t0;
                stack[sp].tMax = d_far; // End at split
                sp++;
            }
        }
    }

    return hit.primitiveIndex != -1;
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

vec3 main_frag(vec2 ndc) {

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
        if (hit.materialID == 0) {
            radiance += throughput * vec3(1, 0.2, 0);
            break;
        }

        // Mirror
        if (hit.materialID == 1) {
            ray.origin = hit.point + hit.normal * EPSILON;
            ray.direction = reflect(ray.direction, hit.normal);
            throughput *= vec3(0.8);
            continue;
        }

        // No material matched, terminate
        break;
    }

    return radiance;
}


layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(binding = 255, rgba8) uniform writeonly image2D resultImage;


void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 screenDims = imageSize(resultImage);

    // Because workgroups are fixed size (e.g., 16x16), the total threads might 
    // slightly exceed the image size. We must return early to avoid writing out of bounds.
    if (pixelCoords.x >= screenDims.x || pixelCoords.y >= screenDims.y) {
        return;
    }

    // center the ray in the pixel.
    vec2 uv = (vec2(pixelCoords) + 0.5) / vec2(screenDims);

    // conversion from [0,1] UV to [-1,1] Clip Space for ray direction
    vec2 ndc = uv * 2.0 - 1.0; 
    
    // Correct for Aspect Ratio
    ndc.y *= -1.0;
    ndc.y *= u_aspectRatio;

    // Raytrace
    vec3 pixelColor = main_frag(ndc);

    // Write Output
    // Format must match the image layout (rgba8 -> vec4)
    imageStore(resultImage, pixelCoords, vec4(pixelColor, 1.0));
}
