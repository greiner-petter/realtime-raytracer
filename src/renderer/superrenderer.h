#ifndef SUPERRENDERER_H
#define SUPERRENDERER_H

#include "renderer/renderer.h"
#include <atomic>

class SuperRenderer : public Renderer {
  static void renderThread(const Scene *scene, Camera const *camera, Texture *image, int width, int widthStep,
                           int widthOffset, int height, int heightStep, int heightOffset, std::atomic<int> *k,
                           int const stepSize, int superSamplingFactor);

public:
  // Constructor / Destructor
  SuperRenderer() = default;
  ~SuperRenderer() override = default;

  // Get
  int superSamplingFactor() { return this->superSamplingFactor_; }

  // Set
  void setSuperSamplingFactor(int factor) { this->superSamplingFactor_ = factor; }

  // Render functions
  Texture renderImage(Scene const &scene, Camera const &camera, int width, int height) override;

private:
  int superSamplingFactor_ = 2;
};

#endif
