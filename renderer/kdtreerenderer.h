#ifndef KDTREERENDERER_H
#define KDTREERENDERER_H

#include "renderer/renderer.h"

// Forward Declaration
class FastScene;

class KDTreeRenderer : public Renderer {

public:
  // Constructor / Destructor
  KDTreeRenderer() = default;
  ~KDTreeRenderer() override = default;

  // Render functions
  Texture renderImage(Scene const &scene, Camera const &camera, int width, int height) override;

  Texture renderKDTree(FastScene const &scene, Camera const &camera, int width, int height);
};

#endif