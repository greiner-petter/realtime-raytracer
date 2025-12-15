#ifndef PERSPECTIVECAMERA_H
#define PERSPECTIVECAMERA_H

#include "camera/camera.h"

class PerspectiveCamera : public Camera {

public:
  // Constructor / Destructor
  PerspectiveCamera();
  ~PerspectiveCamera() override = default;

  // Set
  void setPosition(Vector3d const &position) { this->position = position; }
  void setForwardDirection(Vector3d const &forwardDirection) {
    // Set up a left-handed coordinate system,
    // in which the camera looks along the positive z-Axis
    std::tie(this->forwardDirection, this->upDirection, this->rightDirection) = orthoNormalized(forwardDirection, this->upDirection, crossProduct(this->upDirection, forwardDirection));
  }
  void setUpDirection(Vector3d const &upDirection) {
    // Set up a left-handed coordinate system,
    // in which the camera looks along the positive z-Axis
    std::tie(this->forwardDirection, this->upDirection, this->rightDirection) = orthoNormalized(this->forwardDirection, upDirection, crossProduct(upDirection, this->forwardDirection));
  }
  void setFovAngle(float fovAngle) {
    // Calculate the focus
    this->focus = 1.0f / std::tan((fovAngle * PI / 180) / 2.0f);
  }

  // Camera functions
  Ray createRay(float x, float y) const override;

protected:
  Vector3d position;
  Vector3d forwardDirection;
  Vector3d upDirection;
  Vector3d rightDirection;
  float focus;
};

#endif
