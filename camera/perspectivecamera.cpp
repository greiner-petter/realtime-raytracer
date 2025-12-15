#include "camera/perspectivecamera.h"

PerspectiveCamera::PerspectiveCamera() : forwardDirection(0, 0, 1), upDirection(0, 1, 0), rightDirection(1, 0, 0) { setFovAngle(70); }

Ray PerspectiveCamera::createRay(float x, float y) const {
  // Create the ray
  return Ray(this->position, x * this->rightDirection + y * this->upDirection + focus * this->forwardDirection);
}
