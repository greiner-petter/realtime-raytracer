#ifndef CAMERA_H
#define CAMERA_H

#include "common/Types.h"

void CameraUpdate(class Scene& scene, float deltaTime);
void SetCameraOrientation(const Vec3& forward, const Vec3& up);
void SetCameraForward(const Vec3& forward);
void SetCameraUp(const Vec3& up);
void SetCameraPosition(const Vec3& position);
void SetCameraFOV(float angleInDegrees);

#endif