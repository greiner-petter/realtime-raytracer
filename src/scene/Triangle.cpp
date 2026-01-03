#include "Triangle.h"

float Triangle::minimumBounds(int dimension) const { return std::min(this->vertex[0][dimension], std::min(this->vertex[1][dimension], this->vertex[2][dimension])); }

float Triangle::maximumBounds(int dimension) const { return std::max(this->vertex[0][dimension], std::max(this->vertex[1][dimension], this->vertex[2][dimension])); }
