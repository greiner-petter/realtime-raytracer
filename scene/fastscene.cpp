#include "primitive/primitive.h"
#include "scene/fastscene.h"
#include "shader/shader.h"
#include <algorithm>
#include <iostream>

int Node::countNodeIntersections(const Ray &ray, float t0, float t1) const {
  // If this is a leaf node, we return 0
  if (isLeaf()) {
    return 0;
  } else {
    // Determine the order in which we intersect the child nodes
    float const d = (this->split - ray.origin[this->dimension]) / ray.direction[this->dimension];
    int front = ray.direction[this->dimension] < 0 ? 1 : 0;
    int back = 1 - front;

    if (d <= t0 || d < 0) {
      // t0..t1 is totally behind d, only go through the back node.
      return this->child[back]->countNodeIntersections(ray, t0, t1);
    } else if (d >= t1) {
      // t0..t1 is totally in front of d, only go to front node.
      return this->child[front]->countNodeIntersections(ray, t0, t1);
    } else {
      // Traverse *both* children. Front node first, back node last.
      // Be sure to get even triangles which are coincident with the splitting plane
      return 1 + this->child[front]->countNodeIntersections(ray, t0, d + SPLT_EPS) + this->child[back]->countNodeIntersections(ray, d - SPLT_EPS, t1);
    }
  }
}

bool Node::findIntersection(Ray &ray, float t0, float t1) const {
  // If this is a leaf node, we intersect with all the primitives...
  if (isLeaf()) {
    bool hit = false;
    for (const auto &primitive : this->primitives)
      hit |= primitive->intersect(ray);
    return hit;
  } else { // ... otherwise we continue through the branches

    // Determine the order in which we intersect the child nodes
    float const d = (this->split - ray.origin[this->dimension]) / ray.direction[this->dimension];
    int front = ray.direction[this->dimension] < 0 ? 1 : 0;
    int back = 1 - front;

    if (d <= t0 || d < 0) {
      // t0..t1 is totally behind d, only go through the back node.
      return this->child[back]->findIntersection(ray, t0, t1);
    } else if (d >= t1) {
      // t0..t1 is totally in front of d, only go to front node.
      return this->child[front]->findIntersection(ray, t0, t1);
    } else {
      // Traverse *both* children. Front node first and then back node, if no hit in front of splitting plane.
      // for whatever reason, this doesn't work in the fireplace scene?!
      return ((this->child[front]->findIntersection(ray, t0 - SPLT_EPS, d + SPLT_EPS) && ray.length <= d + SPLT_EPS) || this->child[back]->findIntersection(ray, d - SPLT_EPS, t1 + SPLT_EPS));
      // return (this->child[front]->findIntersection(ray, t0 - SPLT_EPS, d + SPLT_EPS) | this->child[back]->findIntersection(ray, d - SPLT_EPS, t1 + SPLT_EPS));
    }
  }
}

bool Node::findOcclusion(Ray &ray, float t0, float t1) const {
  // If this is a leaf node, we intersect with all the primitives...
  if (isLeaf()) {
    float const rayLength = ray.length;
    for (const auto &primitive : this->primitives)
      // since this is correct, but terribly slow:...
      // if (!primitive->shader()->isTransparent() && primitive->intersect(ray))
      //   return true;
      // we do it this way:
      if (primitive->intersect(ray)) {
        if (!primitive->shader()->isTransparent())
          return true;
        else
          ray.length = rayLength;
      }
    return false;
  } else { // ... otherwise we continue through the branches

    // Determine the order in which we intersect the child nodes
    float const d = (this->split - ray.origin[this->dimension]) / ray.direction[this->dimension];
    int front = ray.direction[this->dimension] < 0 ? 1 : 0;
    int back = 1 - front;

    if (d <= t0 || d < 0) {
      // t0..t1 is totally behind d, only go through the back node.
      return this->child[back]->findOcclusion(ray, t0, t1);
    } else if (d >= t1) {
      // t0..t1 is totally in front of d, only go to front node.
      return this->child[front]->findOcclusion(ray, t0, t1);
    } else {
      // Traverse *both* children. Front node first and then back node, if no hit in front of splitting plane.
      // for whatever reason, this doesn't work in the fireplace scene?!
      return ((this->child[front]->findOcclusion(ray, t0, d + SPLT_EPS) && ray.length <= d + SPLT_EPS) || this->child[back]->findOcclusion(ray, d - SPLT_EPS, t1));
      // return (this->child[front]->findOcclusion(ray, t0, d + SPLT_EPS) | this->child[back]->findOcclusion(ray, d - SPLT_EPS, t1));
    }
  }
}

int FastScene::countNodeIntersections(const Ray &ray) const {
  // Make sure the tree is set up
  if (!this->root)
    return false;

  // Bounding box intersection
  Vector3d const min = componentQuotient(this->absoluteMinimum - ray.origin, ray.direction);
  Vector3d const max = componentQuotient(this->absoluteMaximum - ray.origin, ray.direction);
  float const tMin = std::max(std::max(std::min(min.x, max.x), std::min(min.y, max.y)), std::min(min.z, max.z));
  float const tMax = std::min(std::min(std::max(min.x, max.x), std::max(min.y, max.y)), std::max(min.z, max.z));

  // Traverse the tree recursively
  if (0 <= tMax && tMin <= tMax)
    return this->root->countNodeIntersections(ray, tMin, tMax);
  else
    return 0;
}

bool FastScene::findIntersection(Ray &ray) const {
  // Make sure the tree is set up
  if (!this->root)
    return false;

  // Bounding box intersection
  Vector3d const min = componentQuotient(this->absoluteMinimum - ray.origin, ray.direction);
  Vector3d const max = componentQuotient(this->absoluteMaximum - ray.origin, ray.direction);
  float const tMin = std::max(std::max(std::min(min.x, max.x), std::min(min.y, max.y)), std::min(min.z, max.z));
  float const tMax = std::min(std::min(std::max(min.x, max.x), std::max(min.y, max.y)), std::max(min.z, max.z));

  // Traverse the tree recursively
  if (0 <= tMax && tMin <= tMax)
    return this->root->findIntersection(ray, tMin, tMax);
  else
    return false;
}

bool FastScene::findOcclusion(Ray &ray) const {
  // Make sure the tree is set up
  if (!this->root)
    return false;

  // Bounding box intersection
  Vector3d const min = componentQuotient(this->absoluteMinimum - ray.origin, ray.direction);
  Vector3d const max = componentQuotient(this->absoluteMaximum - ray.origin, ray.direction);
  float const tMin = std::max(std::max(std::min(min.x, max.x), std::min(min.y, max.y)), std::min(min.z, max.z));
  float const tMax = std::min(std::min(std::max(min.x, max.x), std::max(min.y, max.y)), std::max(min.z, max.z));

  // Traverse the tree recursively
  if (0 <= tMax && tMin <= tMax)
    return this->root->findOcclusion(ray, tMin, tMax);
  else
    return false;
}

void FastScene::buildTree(int maximumDepth, int minimumNumberOfPrimitives) {
  // Set the new depth and number of primitives
  this->maximumDepth = maximumDepth;
  this->minimumNumberOfPrimitives = minimumNumberOfPrimitives;

  // Determine the bounding box of the kD-Tree
  this->absoluteMinimum = Vector3d(+INFINITY, +INFINITY, +INFINITY);
  this->absoluteMaximum = Vector3d(-INFINITY, -INFINITY, -INFINITY);
  for (const auto &primitive : this->primitives()) {
    for (int d = 0; d < 3; ++d) {
      this->absoluteMinimum[d] = std::min(this->absoluteMinimum[d], primitive->minimumBounds(d));
      this->absoluteMaximum[d] = std::max(this->absoluteMaximum[d], primitive->maximumBounds(d));
    }
  }

  // Recursively build the kD-Tree
  root = this->build(this->absoluteMinimum, this->absoluteMaximum, this->primitives(), 0);
  std::cout << "(FastScene): " << this->primitives().size() << " primitives organized into tree" << std::endl;
}

std::unique_ptr<Node> FastScene::build(Vector3d const &minimumBounds, Vector3d const &maximumBounds, const std::vector<std::shared_ptr<Primitive>> &primitives, int depth) {
  // Determine the diameter of the bounding box
  Vector3d const diameter = maximumBounds - minimumBounds;

  // Test whether we have reached a leaf node...
  int minimumDimension = ((diameter.x < diameter.y) ? ((diameter.x < diameter.z) ? 0 : 2) : ((diameter.y < diameter.z) ? 1 : 2));
  if (depth >= this->maximumDepth || (int)primitives.size() <= this->minimumNumberOfPrimitives || (diameter[minimumDimension]) <= EPSILON) {
    auto leafNode = std::make_unique<Node>();
    leafNode->primitives = primitives;
    return leafNode;
  }

  // ... otherwise create a new inner node by splitting through the widest dimension
  auto node = std::make_unique<Node>();
  node->dimension = ((diameter.x > diameter.y) ? ((diameter.x > diameter.z) ? 0 : 2) : ((diameter.y > diameter.z) ? 1 : 2));

  // Determine the split position
  // Note: Use the median of the minimum bounds of the primitives
  std::vector<float> minimumValues;
  for (const auto &primitive : primitives)
    minimumValues.push_back(primitive->minimumBounds(node->dimension));
  std::sort(minimumValues.begin(), minimumValues.end());
  node->split = minimumValues[minimumValues.size() / 2];

  // Divide primitives into the left and right lists
  // Remember: A primitive can be in both lists!
  // Also remember: You split exactly at the minimum of a primitive,
  // make sure *that* primitive does *not* appear in both lists!
  std::vector<std::shared_ptr<Primitive>> leftPrimitives, rightPrimitives;
  for (const auto &primitive : primitives) {
    if (primitive->minimumBounds(node->dimension) < node->split)
      leftPrimitives.push_back(primitive);
    if (primitive->maximumBounds(node->dimension) >= node->split)
      rightPrimitives.push_back(primitive);
  }

  // Print out the number of primitives in the left and right child node
  // std::cout << "(FastScene): Split " << leftPrimitives.size() << " | " << rightPrimitives.size() << std::endl;

  // Set the left and right split vectors
  Vector3d minimumSplit = minimumBounds;
  Vector3d maximumSplit = maximumBounds;
  minimumSplit[node->dimension] = node->split;
  maximumSplit[node->dimension] = node->split;

  // Recursively build the tree
  depth += 1;
  node->child[0] = this->build(minimumBounds, maximumSplit, leftPrimitives, depth);
  node->child[1] = this->build(minimumSplit, maximumBounds, rightPrimitives, depth);
  return node;
}
