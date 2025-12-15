#include "common/vector2d.h"
#include "common/common.h"
#include <algorithm>
#include <cassert>

// Access operators ////////////////////////////////////////////////////////////

float &Vector2d::operator[](int dimension) {
  assert(0 <= dimension && dimension < 2);
  switch (dimension) {
  case Dimension::U:
    return this->u;
  case Dimension::V:
    return this->v;
  default: // This must never happen
    return this->u;
  }
}

float const &Vector2d::operator[](int dimension) const {
  assert(0 <= dimension && dimension < 2);
  switch (dimension) {
  case Dimension::U:
    return this->u;
  case Dimension::V:
    return this->v;
  default: // This must never happen
    return this->u;
  }
}

// Comparison operators ////////////////////////////////////////////////////////

bool operator==(Vector2d const &left, Vector2d const &right) { return (left.u == right.u && left.v == right.v); }

bool operator!=(Vector2d const &left, Vector2d const &right) { return !(left == right); }

// Arithmetic operators ////////////////////////////////////////////////////////

Vector2d operator+(Vector2d const &left, Vector2d const &right) { return Vector2d(left.u + right.u, left.v + right.v); }

Vector2d operator-(Vector2d const &right) { return Vector2d(-right.u, -right.v); }

Vector2d operator-(Vector2d const &left, Vector2d const &right) { return Vector2d(left.u - right.u, left.v - right.v); }

Vector2d operator*(Vector2d const &left, float right) { return Vector2d(left.u * right, left.v * right); }

Vector2d operator*(float left, Vector2d const &right) { return Vector2d(left * right.u, left * right.v); }

Vector2d operator/(Vector2d const &left, float right) { return Vector2d(left.u / right, left.v / right); }

// Assignment operators ////////////////////////////////////////////////////////

Vector2d &operator+=(Vector2d &left, Vector2d const &right) {
  left.u += right.u;
  left.v += right.v;
  return left;
}

Vector2d &operator-=(Vector2d &left, Vector2d const &right) {
  left.u -= right.u;
  left.v -= right.v;
  return left;
}

Vector2d &operator*=(Vector2d &left, float right) {
  left.u *= right;
  left.v *= right;
  return left;
}

Vector2d &operator/=(Vector2d &left, float right) {
  left.u /= right;
  left.v /= right;
  return left;
}

// Useful functions ////////////////////////////////////////////////////////////

Vector2d componentProduct(const Vector2d &left, const Vector2d &right) { return Vector2d(left.u * right.u, left.v * right.v); }

Vector2d componentQuotient(const Vector2d &left, const Vector2d &right) { return Vector2d(left.u / right.u, left.v / right.v); }

float dotProduct(Vector2d const &left, Vector2d const &right) { return left.u * right.u + left.v * right.v; }

float length(Vector2d const &c) { return std::sqrt(dotProduct(c, c)); }

Vector2d normalized(Vector2d const &v) { return v / std::max(length(v), NORM_EPS); }

void normalize(Vector2d *v) { *v = normalized(*v); }
