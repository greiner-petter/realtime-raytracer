#include "primitive/triangle.h"
#include <algorithm>

// Constructor /////////////////////////////////////////////////////////////////

Triangle::Triangle(std::shared_ptr<Shader> const &shader) : Primitive(shader) {}

Triangle::Triangle(Vector3d const &a, Vector3d const &b, Vector3d const &c, std::shared_ptr<Shader> const &shader) : Primitive(shader), vertex{a, b, c} {}

Triangle::Triangle(Vector3d const &a, Vector3d const &b, Vector3d const &c, Vector3d const &na, Vector3d const &nb, Vector3d const &nc, std::shared_ptr<Shader> const &shader) : Primitive(shader), vertex{a, b, c}, normal{na, nb, nc} {}

Triangle::Triangle(Vector3d const &a, Vector3d const &b, Vector3d const &c, Vector3d const &na, Vector3d const &nb, Vector3d const &nc, Vector2d const &ta, Vector2d const &tb, Vector2d const &tc, std::shared_ptr<Shader> const &shader)
    : Primitive(shader), vertex{a, b, c}, normal{na, nb, nc}, surface{ta, tb, tc} {}

Triangle::Triangle(Vector3d const &a, Vector3d const &b, Vector3d const &c, Vector3d const &na, Vector3d const &nb, Vector3d const &nc, Vector3d const &tana, Vector3d const &tanb, Vector3d const &tanc, Vector3d const &ba,
                   Vector3d const &bb, Vector3d const &bc, Vector2d const &ta, Vector2d const &tb, Vector2d const &tc, std::shared_ptr<Shader> const &shader)
    : Primitive(shader), vertex{a, b, c}, normal{na, nb, nc}, tangent{tana, tanb, tanc}, bitangent{ba, bb, bc}, surface{ta, tb, tc} {}

// Primitive functions /////////////////////////////////////////////////////////

bool Triangle::intersectArea(Ray &ray) const {
  // alternative triangle test
  // "signed" triangle area with respect to triangle normal
  auto triangleArea = [](Vector3d const &v0, Vector3d const &v1, Vector3d const &v2, Vector3d const &normal = Vector3d(0, 0, 0)) {
    if (length(normal) < EPSILON) {
      return length(crossProduct(v2 - v0, v1 - v0)) / 2.0f;
    } else {
      Vector3d const cp = crossProduct(v2 - v0, v1 - v0);
      return dotProduct(cp, normal) > 0.0f ? length(cp) / 2.0f : -length(cp) / 2.0f;
    }
  };

  // begin ray-plane intersection ----------------------------
  Vector3d normal = normalized(crossProduct(vertex[2] - vertex[0], vertex[1] - vertex[0]));

  float const cosine = dotProduct(ray.direction, normal);

  if (abs(cosine) < EPSILON)
    return false;

  float const t = dotProduct(vertex[0] - ray.origin, normal) / cosine;

  if (t < EPSILON || ray.length < t)
    return false;

  Vector3d const p = ray.origin + t * ray.direction;
  // end ray-plane intersection ----------------------------

  float const fullArea = triangleArea(vertex[0], vertex[1], vertex[2]);
  float const a = triangleArea(p, vertex[0], vertex[1], normal) / fullArea;
  float const b = triangleArea(p, vertex[2], vertex[0], normal) / fullArea;

  if ((a < 0.0f) || (a > 1.0f) || (b < 0.0f) || (a + b > 1.0f))
    return false;

  // Set the surface position (barycentric coordinates) and tangent Vector
  ray.surface = a * this->surface[1] + b * this->surface[2] + (1 - a - b) * this->surface[0];

  // Set the new length and the current primitive
  ray.length = t;
  ray.primitive = this;

  // True, because the primitive was hit
  return true;
}

bool Triangle::intersect(Ray &ray) const {
  // We use the Möller–Trumbore intersection algorithm

  // Determine two neighboring edge vectors
  Vector3d const edge1 = this->vertex[1] - this->vertex[0];
  Vector3d const edge2 = this->vertex[2] - this->vertex[0];

  // Begin calculating determinant
  Vector3d const pVec = crossProduct(ray.direction, edge2);

  // Make sure the ray is not parallel to the triangle
  float const det = dotProduct(edge1, pVec);
  if (fabs(det) < EPSILON)
    return false;
  float const inv_det = 1.0f / det;

  // Calculate u and test bound
  Vector3d const tVec = ray.origin - this->vertex[0];
  float const u = dotProduct(tVec, pVec) * inv_det;
  // Test whether the intersection lies outside the triangle
  if (0.0f > u || u > 1.0f)
    return false;

  // Calculate v and test bound
  Vector3d const qVec = crossProduct(tVec, edge1);
  float const v = dotProduct(ray.direction, qVec) * inv_det;
  // Test whether the intersection lies outside the triangle
  if (0.0f > v || u + v > 1.0f)
    return false;

  // Test whether this is the foremost primitive in front of the camera
  float const t = dotProduct(edge2, qVec) * inv_det;
  if (t < EPSILON || ray.length < t)
    return false;

  // Calculate the normal
  if (length(this->normal[0]) * length(this->normal[1]) * length(this->normal[2]) > EPSILON)
    ray.normal = normalized(u * this->normal[1] + v * this->normal[2] + (1 - u - v) * this->normal[0]);
  else
    ray.normal = normalized(crossProduct(edge1, edge2));
  // calculate the tangent and bitangent vectors as well
  ray.tangent = normalized(u * this->tangent[1] + v * this->tangent[2] + (1 - u - v) * this->tangent[0]);
  ray.bitangent = normalized(u * this->bitangent[1] + v * this->bitangent[2] + (1 - u - v) * this->bitangent[0]);

  // Calculate the surface position
  ray.surface = u * this->surface[1] + v * this->surface[2] + (1 - u - v) * this->surface[0];

  // Set the new length and the current primitive
  ray.length = t;
  ray.primitive = this;

  // True, because the primitive was hit
  return true;
}

// Bounding box ////////////////////////////////////////////////////////////////

float Triangle::minimumBounds(int dimension) const { return std::min(this->vertex[0][dimension], std::min(this->vertex[1][dimension], this->vertex[2][dimension])); }

float Triangle::maximumBounds(int dimension) const { return std::max(this->vertex[0][dimension], std::max(this->vertex[1][dimension], this->vertex[2][dimension])); }
