#include "InfinitePlane.h"

float InfinitePlane::minimumBounds(int dimension) const {
  if (this->normal[dimension] == 1.0f) // plane is orthogonal to the axis
    return this->origin[dimension] - EPSILON;
  else
    return -INFINITY;
}

float InfinitePlane::maximumBounds(int dimension) const {
  if (this->normal[dimension] == 1.0f) // plane is orthogonal to the axis
    return this->origin[dimension] + EPSILON;
  else
    return +INFINITY;
}
