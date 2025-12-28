#include "camera/camera.h"
#include "renderer/kdtreerenderer.h"
#include "scene/fastscene.h"
#include <iostream>

Texture KDTreeRenderer::renderImage(Scene const &scene, Camera const &camera, int width, int height) {
  Texture image(width, height);
  std::cout << "KD Tree renderer will only output KD Tree visualization" << std::endl;
  return image;
}

Texture KDTreeRenderer::renderKDTree(FastScene const &scene, Camera const &camera, int width, int height) {
  Texture image(width, height);
  float const aspectRatio = static_cast<float>(height) / width;

  for (int x = 0; x < image.width(); ++x) {
    for (int y = 0; y < image.height(); ++y) {
      Ray ray = camera.createRay((static_cast<float>(x) / width * 2 - 1),
                                 -(static_cast<float>(y) / height * 2 - 1) * aspectRatio);

      image.setPixelAt(x, y, Color(float(scene.countNodeIntersections(ray)), 0.0f, 0.0f));
    }
  }

  // map color to green -> red map
  float maxVal = -INFINITY;
  for (int x = 0; x < image.width(); ++x) {
    for (int y = 0; y < image.height(); ++y) {
      maxVal = std::max(maxVal, image.getPixelAt(x, y).r);
    }
  }

  Color r(1, 0, 0);
  Color g(0, 1, 0);
  for (int x = 0; x < image.width(); ++x) {
    for (int y = 0; y < image.height(); ++y) {
      float factor = image.getPixelAt(x, y).r / maxVal;
      image.setPixelAt(x, y, (1.0f - factor) * g + factor * r);
    }
  }

  return image;
}