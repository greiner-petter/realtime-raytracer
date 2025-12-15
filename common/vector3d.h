#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <tuple>

struct Vector3d {
  // Components
  float x, y, z;

  // Enum for readability
  enum Dimension { X, Y, Z };

  // Constructor
  Vector3d() : x(0), y(0), z(0) {}
  Vector3d(float x, float y, float z) : x(x), y(y), z(z) {}

  // Access operators
  float &operator[](int dimension);
  float const &operator[](int dimension) const;
};

// Comparison operators
bool operator==(Vector3d const &left, Vector3d const &right);
bool operator!=(Vector3d const &left, Vector3d const &right);

// Arithmetic operators
Vector3d operator+(Vector3d const &left, Vector3d const &right);
Vector3d operator-(Vector3d const &right);
Vector3d operator-(Vector3d const &left, Vector3d const &right);
Vector3d operator*(Vector3d const &left, float right);
Vector3d operator*(float left, Vector3d const &right);
Vector3d operator*(Vector3d const &left, Vector3d const &right);
Vector3d operator/(Vector3d const &left, float right);
Vector3d operator/(Vector3d const &left, Vector3d const &right);

// Assignment operators
Vector3d &operator+=(Vector3d &left, Vector3d const &right);
Vector3d &operator-=(Vector3d &left, Vector3d const &right);
Vector3d &operator*=(Vector3d &left, float right);
Vector3d &operator*=(Vector3d &left, Vector3d const &right);
Vector3d &operator/=(Vector3d &left, float right);
Vector3d &operator/=(Vector3d &left, Vector3d const &right);

// Useful functions
Vector3d componentProduct(Vector3d const &left, Vector3d const &right);
Vector3d componentQuotient(Vector3d const &left, Vector3d const &right);
Vector3d crossProduct(Vector3d const &left, Vector3d const &right);
float dotProduct(Vector3d const &left, Vector3d const &right);
float length(Vector3d const &v);
Vector3d normalized(Vector3d const &v);
void normalize(Vector3d *v);
std::tuple<Vector3d, Vector3d, Vector3d> orthoNormalized(Vector3d const &u, Vector3d const &v, Vector3d const &w);

#endif
