#ifndef VECTOR2D_H
#define VECTOR2D_H

struct Vector2d {
  // Components
  float u, v;

  // Enum for readability
  enum Dimension { U, V };

  // Constructor
  Vector2d() : u(0), v(0) {}
  Vector2d(float u, float v) : u(u), v(v) {}

  // Access operators
  float &operator[](int dimension);
  float const &operator[](int dimension) const;
};

// Comparison operators
bool operator==(Vector2d const &left, Vector2d const &right);
bool operator!=(Vector2d const &left, Vector2d const &right);

// Arithmetic operators
Vector2d operator+(Vector2d const &left, Vector2d const &right);
Vector2d operator-(Vector2d const &right);
Vector2d operator-(Vector2d const &left, Vector2d const &right);
Vector2d operator*(Vector2d const &left, float right);
Vector2d operator*(float left, Vector2d const &right);
Vector2d operator/(Vector2d const &left, float right);

// Assignment operators
Vector2d &operator+=(Vector2d &left, Vector2d const &right);
Vector2d &operator-=(Vector2d &left, Vector2d const &right);
Vector2d &operator*=(Vector2d &left, float right);
Vector2d &operator/=(Vector2d &left, float right);

// Useful functions
Vector2d componentProduct(Vector2d const &left, Vector2d const &right);
Vector2d componentQuotient(Vector2d const &left, Vector2d const &right);
float dotProduct(Vector2d const &left, Vector2d const &right);
float length(Vector2d const &c);
Vector2d normalized(Vector2d const &v);
void normalize(Vector2d *v);

#endif
