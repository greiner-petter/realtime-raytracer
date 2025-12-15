#ifndef COLOR_H
#define COLOR_H

struct Color {

  // Components
  float r, g, b;

  // Enum for readability
  enum Channel { R, G, B };

  // Constructor
  Color() : r(0), g(0), b(0) {}
  Color(float r, float g, float b) : r(r), g(g), b(b) {}

  // Access operators
  float &operator[](int channel);
  float const &operator[](int channel) const;
};

// Comparison operators
bool operator==(Color const &left, Color const &right);
bool operator!=(Color const &left, Color const &right);

// Arithmetic operators
Color operator+(Color const &left, Color const &right);
Color operator-(Color const &right);
Color operator-(Color const &left, Color const &right);
Color operator*(Color const &left, float right);
Color operator*(float left, Color const &right);
Color operator*(Color const &left, Color const &right);
Color operator/(Color const &left, float right);
Color operator/(Color const &left, Color const &right);

// Assignment operators
Color &operator+=(Color &left, Color const &right);
Color &operator-=(Color &left, Color const &right);
Color &operator*=(Color &left, float right);
Color &operator/=(Color &left, float right);

// Useful functions
Color clamped(Color const &c);
void clamp(Color *c);

#endif
