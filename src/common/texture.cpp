#include "common/texture.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

Texture::Texture(int width, int height) { this->resize(width, height); }

Texture::Texture(char const *fileName) { this->load(fileName); }

bool Texture::load(char const *fileName) {
  CImg<unsigned char> img_char(fileName);
  if (img_char.is_empty()) {
    std::cerr << "(Image): Could not open file " << fileName << std::endl;
    return false;
  }

  this->image_ = CImg<float>(img_char.width(), img_char.height(), img_char.depth(), 3, 0);
  for (int c = 0; c < 3; c++)
    cimg_forXYZ(img_char, x, y, z) {
      this->image_(x, y, z, c) = img_char(x, y, z, std::min(c, img_char.spectrum() - 1)) / 255.0f;
    }

  return true;
}

bool Texture::save(char const *fileName) const {
  CImg<unsigned char> img_char;
  img_char = 255 * this->image_;

  img_char.save(fileName);
  return true;
}

Color Texture::getPixelAt(int x, int y) const {
  if ((x %= this->width()) < 0)
    x += this->width();
  if ((y %= this->height()) < 0)
    y += this->height();
  return Color(this->image_.atXY(x, y, 0), this->image_.atXY(x, y, 1), this->image_.atXY(x, y, 2));
}

void Texture::setPixelAt(int x, int y, Color const &color) {
  if ((x %= this->width()) < 0)
    x += this->width();
  if ((y %= this->height()) < 0)
    y += this->height();
  this->image_(x, y, 0) = color.r;
  this->image_(x, y, 1) = color.g;
  this->image_(x, y, 2) = color.b;
}

Color Texture::color(float u, float v, bool interpolate) const {
  Color color;
  if (!interpolate) {
    color = this->getPixelAt(int(roundf(u * this->width())), int(roundf(v * this->height())));
  } else {
    // bilinear interpolation
    // adjacent pixel coordinates
    int left = int(floorf(u * this->width()));
    int right = int(ceilf(u * this->width()));
    int top = int(floorf(v * this->height()));
    int bottom = int(ceilf(v * this->height()));

    // weights
    float w[4];
    w[0] = right - u * this->width();
    w[1] = 1 - w[0];
    w[2] = bottom - v * this->height();
    w[3] = 1 - w[2];

    // get color values and interpolate
    Color val[4];
    val[0] = this->getPixelAt(left, top);
    val[1] = this->getPixelAt(right, top);
    val[2] = this->getPixelAt(left, bottom);
    val[3] = this->getPixelAt(right, bottom);
    color = w[2] * w[0] * val[0] + w[2] * w[1] * val[1] + w[3] * w[0] * val[2] + w[3] * w[1] * val[3];
  }
  return color;
}

Color Texture::color(Vector2d const &surfacePosition, bool interpolate) const {
  return color(surfacePosition.u, surfacePosition.v, interpolate);
}
