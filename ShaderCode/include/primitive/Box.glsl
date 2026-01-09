struct Box {
    vec4 center;
    vec4 size;
};

layout(binding = 14, std430) buffer Boxes {
    uint BoxCount;
    Box boxes[];
};

vec3 componentQuotient(vec3 left, vec3 right) { return vec3(left.x / right.x, left.y / right.y, left.z / right.z); }

bool intersectBox(Ray ray, Box box, inout Hit hit) {
  // Project the ray onto the box
  vec3 minBounds = box.center.xyz - box.size.xyz / 2;
  vec3 maxBounds = box.center.xyz + box.size.xyz / 2;
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
  float t = (tNear >= 0 ? tNear : tFar);
  int tIndex = tNear >= 0 ? tNearIndex : tFarIndex;

  // Test whether this is the foremost primitive in front of the camera
  if (hit.rayLength < t)
    return false;

  // Calculate the normal
  hit.normal = vec3(0, 0, 0);
  // Flip the normal if we are on the inside
  // Note: This is necessary to ensure we don't backface-cull an environment cube; for compatibility with
  //       the refraction shader, the normal should *always* point outwards.
  hit.normal[tIndex] = sign(ray.direction[tIndex]) * (tNear < 0.0 ? 1.0 : -1.0);

  // Calculate the surface position and tangent vector
  vec3 target = ray.origin + t * ray.direction;
  vec3 surface = componentQuotient(target - minBounds, maxBounds - minBounds);
  if (tIndex == 0) {
    hit.surface = vec2(surface[2], surface[1]);
    hit.tangent = vec3(0, 0, 1);
  } else if (tIndex == 1) {
    hit.surface = vec2(surface[0], surface[2]);
    hit.tangent = vec3(1, 0, 0);
  } else {
    hit.surface = vec2(surface[0], surface[1]);
    hit.tangent = vec3(1, 0, 0);
  }

  // Set the new length and the current primitive
  hit.rayLength = t;
  hit.point = target;

  // True, because the primitive was hit
  return true;
}
