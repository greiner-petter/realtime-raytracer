struct Triangle {
    vec4 vertex[3];
    vec4 normal[3];
    vec4 tangent[3];
    vec4 bitangent[3];
    vec4 surface[3];
};

layout(binding = 12, std430) buffer Triangles {
    uint triangleCount;
    Triangle triangles[];
};

bool intersectTriangle(inout  Ray ray, in Primitive primitive) {
    const Triangle triangle = triangles[primitive.primitiveIndex];
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
    ray.primitive = primitive;

    // True, because the primitive was hit
    return true;
}
