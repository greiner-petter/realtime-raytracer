#ifndef SIMPLERENDERER_H
#define SIMPLERENDERER_H

#include <atomic>
#include "renderer/renderer.h"

class SimpleRenderer : public Renderer {
  static void renderThread(const Scene *scene, Camera const *camera, Texture *image, int width, int widthStep,
                           int widthOffset, int height, int heightStep, int heightOffset, std::atomic<int> *k,
                           int const stepSize);

public:
  // Constructor / Destructor
  SimpleRenderer() = default;
  ~SimpleRenderer() override = default;

  // Render functions
  Texture renderImage(Scene const &scene, Camera const &camera, int width, int height) override;
};

#endif
