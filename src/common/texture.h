#ifndef TEXTURE_H
#define TEXTURE_H

#include "common/CImg.h"
#include "common/color.h"
#include "common/vector2d.h"

using namespace cimg_library;

class Texture {
public:
  // Constructor
  Texture(int width, int height);
  Texture(char const *fileName);

  // Image functions
  inline void resize(int width, int height) { this->image_.resize(width, height, 1, 3); }
  bool load(char const *fileName);
  bool save(char const *fileName) const;

  // Get
  inline bool isNull() const { return this->image_.is_empty(); }
  inline int width() const { return this->image_.width(); }
  inline int height() const { return this->image_.height(); }
  Color getPixelAt(int x, int y) const;

  // Set
  void setPixelAt(int x, int y, Color const &color);

  // Color functions
  Color color(float u, float v, bool interpolate = true) const;
  Color color(Vector2d const &surfacePosition, bool interpolate = true) const;

private:
  CImg<float> image_;
};

#endif
