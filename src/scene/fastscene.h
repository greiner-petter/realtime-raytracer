#ifndef FASTSCENE_H
#define FASTSCENE_H

#include "scene/scene.h"

struct Node {
  // Constructor / Destructor
  Node() : dimension(0), split(0) {}

  // Traversal function
  bool findIntersection(Ray &ray, float t0, float t1) const;
  bool findOcclusion(Ray &ray, float t0, float t1) const;
  int countNodeIntersections(const Ray &ray, float t0, float t1) const;
  inline bool isLeaf() const { return (!this->primitives.empty() || (!this->child[0] && !this->child[1])); }

  // Branch split
  std::unique_ptr<Node> child[2];
  int dimension;
  float split;

  // Leaf primitives
  std::vector<std::shared_ptr<Primitive>> primitives;
};

class FastScene : public Scene {

public:
  // Raytracing functions
  bool findIntersection(Ray &ray) const override;
  bool findOcclusion(Ray &ray) const override;
  int countNodeIntersections(const Ray &ray) const;

  // Setup functions
  void buildTree(int maximumDepth = 10, int minimumNumberOfPrimitives = 2);

private:
  std::unique_ptr<Node> build(Vector3d const &minimumBounds, Vector3d const &maximumBounds,
                              const std::vector<std::shared_ptr<Primitive>> &primitives, int depth);

  std::unique_ptr<Node> root;
  int maximumDepth;
  int minimumNumberOfPrimitives;
  Vector3d absoluteMinimum, absoluteMaximum;
};

#endif
