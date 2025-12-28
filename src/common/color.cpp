#include "common/color.h"
#include <algorithm>
#include <cassert>

// Access operators ////////////////////////////////////////////////////////////

float &Color::operator[](int channel) {
  assert(0 <= channel && channel < 3);
  switch (channel) {
  case Channel::R:
    return this->r;
  case Channel::G:
    return this->g;
  case Channel::B:
    return this->b;
  default: // This must never happen
    return this->r;
  }
}

float const &Color::operator[](int channel) const {
  assert(0 <= channel && channel < 3);
  switch (channel) {
  case Channel::R:
    return this->r;
  case Channel::G:
    return this->g;
  case Channel::B:
    return this->b;
  default: // This must never happen
    return this->r;
  }
}

// Comparison operators ////////////////////////////////////////////////////////

bool operator==(Color const &left, Color const &right) {
  return (left.r == right.r && left.g == right.g && left.b == right.b);
}

bool operator!=(Color const &left, Color const &right) { return !(left == right); }

// Arithmetic operators ////////////////////////////////////////////////////////

Color operator+(Color const &left, Color const &right) {
  return Color(left.r + right.r, left.g + right.g, left.b + right.b);
}

Color operator-(Color const &right) { return Color(-right.r, -right.g, -right.b); }

Color operator-(Color const &left, Color const &right) {
  return Color(left.r - right.r, left.g - right.g, left.b - right.b);
}

Color operator*(Color const &left, float right) { return Color(left.r * right, left.g * right, left.b * right); }

Color operator*(float left, Color const &right) { return Color(left * right.r, left * right.g, left * right.b); }

Color operator*(Color const &left, Color const &right) {
  return Color(left.r * right.r, left.g * right.g, left.b * right.b);
}

Color operator/(Color const &left, float right) { return Color(left.r / right, left.g / right, left.b / right); }

Color operator/(Color const &left, Color const &right) {
  return Color(left.r / right.r, left.g / right.g, left.b / right.b);
}

// Assignment operators ////////////////////////////////////////////////////////

Color &operator+=(Color &left, Color const &right) {
  left.r += right.r;
  left.g += right.g;
  left.b += right.b;
  return left;
}

Color &operator-=(Color &left, Color const &right) {
  left.r -= right.r;
  left.g -= right.g;
  left.b -= right.b;
  return left;
}

Color &operator*=(Color &left, float right) {
  left.r *= right;
  left.g *= right;
  left.b *= right;
  return left;
}

Color &operator/=(Color &left, float right) {
  left.r /= right;
  left.g /= right;
  left.b /= right;
  return left;
}

// Useful functions ////////////////////////////////////////////////////////////

Color clamped(Color const &c) {
  return Color(std::max(0.0f, std::min(c.r, 1.0f)), std::max(0.0f, std::min(c.g, 1.0f)),
               std::max(0.0f, std::min(c.b, 1.0f)));
}

void clamp(Color *c) { *c = clamped(*c); }
