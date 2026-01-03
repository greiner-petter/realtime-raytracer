bool intersectTriangle(Ray ray, Triangle triangle, inout Hit hit) {
  // We use the Möller–Trumbore intersection algorithm

  // Determine two neighboring edge vectors
  vec3 edge1 = triangle.vertex[1].xyz - triangle.vertex[0].xyz;
  vec3 edge2 = triangle.vertex[2].xyz - triangle.vertex[0].xyz;

  // Begin calculating determinant
  vec3 pVec = cross(ray.direction, edge2);

  // Make sure the ray is not parallel to the triangle
  float det = dot(edge1, pVec);
  if (abs(det) < EPSILON)
    return false;
  float inv_det = 1.0 / det;

  // Calculate u and test bound
  vec3 tVec = ray.origin - triangle.vertex[0].xyz;
  float u = dot(tVec, pVec) * inv_det;
  // Test whether the intersection lies outside the triangle
  if (0.0 > u || u > 1.0)
    return false;

  // Calculate v and test bound
  vec3 qVec = cross(tVec, edge1);
  float v = dot(ray.direction, qVec) * inv_det;
  // Test whether the intersection lies outside the triangle
  if (0.0 > v || u + v > 1.0)
    return false;

  // Test whether this is the foremost primitive in front of the camera
  float t = dot(edge2, qVec) * inv_det;
  if (t < EPSILON || hit.rayLength < t)
    return false;

  // Calculate the normal
  if (length(triangle.normal[0].xyz) * length(triangle.normal[1].xyz) * length(triangle.normal[2].xyz) > EPSILON)
    hit.normal = normalize(u * triangle.normal[1].xyz + v * triangle.normal[2].xyz + (1 - u - v) * triangle.normal[0].xyz);
  else
    hit.normal = normalize(cross(edge1, edge2));
  // calculate the tangent and bitangent vectors as well
  hit.tangent = normalize(u * triangle.tangent[1].xyz + v * triangle.tangent[2].xyz + (1 - u - v) * triangle.tangent[0].xyz);
  hit.bitangent = normalize(u * triangle.bitangent[1].xyz + v * triangle.bitangent[2].xyz + (1 - u - v) * triangle.bitangent[0].xyz);

  // Calculate the surface position
  hit.surface = u * triangle.surface[1].xy + v * triangle.surface[2].xy + (1 - u - v) * triangle.surface[0].xy;
  // Set the new length and the current primitive
  hit.rayLength = t;
  hit.point = ray.origin + t * ray.direction;

  // True, because the primitive was hit
  return true;
}