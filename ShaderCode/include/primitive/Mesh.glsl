struct Mesh {
    vec4 minBounds_index;
    vec4 maxBounds_count;
};

layout(binding = 3, std430) buffer MeshTriangles {
    uint meshTriangleCount;
    Triangle meshTriangles[];
};

layout(binding = 15, std430) buffer Meshes {
    uint meshCount;
    Mesh meshes[];
};

bool intersectBoundingBox(vec3 minBounds, vec3 maxBounds, Ray ray) {
    vec3 t1 = componentQuotient(minBounds - ray.origin, ray.direction);
    vec3 t2 = componentQuotient(maxBounds - ray.origin, ray.direction);

    // Determine the intersection points (tNear, tFar)
    // We also have to remember the intersection axes (tNearIndex, tFarIndex)
    float tNear = -INFINITY;
    float tFar = +INFINITY;
    int tNearIndex = 0;
    int tFarIndex = 0;
    for (int d = 0; d < 3; ++d) {

        // Test the trivial case (and to avoid division by zero errors)
        if (ray.direction[d] == 0 && (ray.origin[d] < minBounds[d] || ray.origin[d] > maxBounds[d]))
            return false;

        // Swap the bounds if necessary
        if (t1[d] > t2[d]) {
            float temp = t1[d];
            t1[d] = t2[d];
            t2[d] = temp;
        }

        // Check for the near intersection
        if (t1[d] > tNear) {
            tNear = t1[d];
            tNearIndex = d;
        }

        // Check for the far intersection
        if (t2[d] < tFar) {
            tFar = t2[d];
            tFarIndex = d;
        }

        // Check whether we missed the box completely
        if (tFar < 0 || tNear > tFar)
            return false;
    }

    // Check whether we are on the outside or on the inside of the box
    const float t = (tNear >= 0 ? tNear : tFar);
    const int tIndex = tNear >= 0 ? tNearIndex : tFarIndex;

    // Test whether this is the foremost primitive in front of the camera
    if (ray.rayLength < t)
        return false;

    
    return true;
}

bool intersectMeshTriangle(inout Ray ray, in Triangle triangle) {
    // We use the Möller–Trumbore intersection algorithm

    // Determine two neighboring edge vectors
    const vec3 edge1 = triangle.vertex[1].xyz - triangle.vertex[0].xyz;
    const vec3 edge2 = triangle.vertex[2].xyz - triangle.vertex[0].xyz;

    // Begin calculating determinant
    const vec3 pVec = cross(ray.direction, edge2);

    // Make sure the ray is not parallel to the triangle
    const float det = dot(edge1, pVec);
    if (abs(det) < EPSILON)
        return false;
    const float inv_det = 1.0f / det;

    // Calculate u and test bound
    const vec3 tVec = ray.origin - triangle.vertex[0].xyz;
    const float u = dot(tVec, pVec) * inv_det;
    // Test whether the intersection lies outside the triangle
    if (0.0f > u || u > 1.0f)
        return false;

    // Calculate v and test bound
    const vec3 qVec = cross(tVec, edge1);
    const float v = dot(ray.direction, qVec) * inv_det;
    // Test whether the intersection lies outside the triangle
    if (0.0f > v || u + v > 1.0f)
        return false;

    // Test whether this is the foremost primitive in front of the camera
    const float t = dot(edge2, qVec) * inv_det;
    if (t < EPSILON || ray.rayLength < t)
        return false;

    // Calculate the normal
    if (length(triangle.normal[0].xyz) * length(triangle.normal[1].xyz) * length(triangle.normal[2].xyz) > EPSILON)
        ray.normal = normalize(u * triangle.normal[1].xyz + v * triangle.normal[2].xyz + (1 - u - v) * triangle.normal[0].xyz);
    else
        ray.normal = normalize(cross(edge1, edge2));
    // Ensure normal faces the ray origin (flip if pointing away)
    if (dot(ray.normal, ray.direction) > 0.0f) {
        ray.normal = -ray.normal;
    }
    // calculate the tangent and bitangent vectors as well
    ray.tangent = normalize(u * triangle.tangent[1].xyz + v * triangle.tangent[2].xyz + (1 - u - v) * triangle.tangent[0].xyz);
    ray.bitangent = normalize(u * triangle.bitangent[1].xyz + v * triangle.bitangent[2].xyz + (1 - u - v) * triangle.bitangent[0].xyz);

    // Calculate the surface position
    ray.surface = u * triangle.surface[1].xy + v * triangle.surface[2].xy + (1 - u - v) * triangle.surface[0].xy;

    // Set the new length and the current primitive
    ray.rayLength = t;

    // True, because the primitive was hit
    return true;
}

bool intersectMesh(inout Ray ray, in Primitive primitive) {
    const Mesh mesh = meshes[primitive.primitiveIndex];
    
    if (!intersectBoundingBox(mesh.minBounds_index.xyz, mesh.maxBounds_count.xyz, ray)) {
        return false;
    }

    bool hitTriangle = false;
    for (int i = int(mesh.minBounds_index.w); i < int(mesh.minBounds_index.w) + int(mesh.maxBounds_count.w); i++) {
        const Triangle triangle = meshTriangles[i];
        if (intersectMeshTriangle(ray, triangle)) {
            hitTriangle = true;
            ray.primitive = primitive;
        }
    }

    return hitTriangle;
}
