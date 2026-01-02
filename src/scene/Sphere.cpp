#include "Sphere.h"

float Sphere::minimumBounds(int dimension) const { 
    return this->GetCenter()[dimension] - this->GetRadius(); 
}

float Sphere::maximumBounds(int dimension) const { 
    return this->GetCenter()[dimension] + this->GetRadius(); 
}