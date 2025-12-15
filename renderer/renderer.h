#ifndef RENDERER_H
#define RENDERER_H

#include "common/texture.h"

// Forward declarations
class Camera;
class Scene;

class Renderer {

public:
  // Constructor / Destructor
  Renderer() = default;
  virtual ~Renderer() = default;

  // Render functions
  virtual Texture renderImage(Scene const &scene, Camera const &camera, int width, int height) = 0;
};

#endif
