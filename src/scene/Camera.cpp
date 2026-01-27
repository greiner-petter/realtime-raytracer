#include "scene/Camera.h"
#include "common/Input.h"
#include "common/Log.h"
#include "common/Types.h"
#include "common/Params.h"
#include "scene/Scene.h"

extern UBO uniformBufferData;

// Store camera orientation as basis vectors instead of Euler angles
Vec3 cameraForward = VecUtils::Forward;
Vec3 cameraUp = VecUtils::Up;

void SetCameraOrientation(const Vec3& forward, const Vec3& up) {
    Vec3 right = glm::cross(glm::normalize(up), glm::normalize(forward));

    uniformBufferData.u_CameraForward = Vec4(glm::normalize(forward), 0.0f);
    uniformBufferData.u_CameraRight = Vec4(glm::normalize(right), 0.0f);
    uniformBufferData.u_CameraUp = Vec4(glm::normalize(up), 0.0f);

    // Update stored orientation
    cameraForward = glm::normalize(forward);
    cameraUp = glm::normalize(up);
}

void SetCameraForward(const Vec3& forward) {
    cameraForward = glm::normalize(forward);
    SetCameraOrientation(cameraForward, cameraUp);
}

void SetCameraUp(const Vec3& up) {
    cameraUp = glm::normalize(up);
    SetCameraOrientation(cameraForward, cameraUp);
}

void SetCameraPosition(const Vec3& position) {
    uniformBufferData.u_CameraPosition = Vec4(position, 0.0f);
}

void SetCameraFOV(float fovAngle) {
    uniformBufferData.u_FocusDistance = 1.0f / std::tan((fovAngle * PI / 180) / 2.0f);
}

void UpdateCameraDirection(float deltaX, float deltaY, float deltaRoll) {
    const float MOUSE_SENSITIVITY = 0.005f;
    const float ROLL_SPEED = 2.0f;

    // Get current camera basis
    Vec3 right = glm::normalize(glm::cross(cameraUp, cameraForward));

    // Apply yaw rotation (horizontal mouse) around current up axis
    if (deltaX != 0.0f) {
        float yawAngle = -deltaX * MOUSE_SENSITIVITY;
        Mat3 yawRotation = Mat3(glm::rotate(Mat4(1.0f), yawAngle, cameraUp));
        cameraForward = glm::normalize(yawRotation * cameraForward);
        right = glm::normalize(yawRotation * right);
    }

    // Apply pitch rotation (vertical mouse) around current right axis
    if (deltaY != 0.0f) {
        float pitchAngle = -deltaY * MOUSE_SENSITIVITY;
        Mat3 pitchRotation = Mat3(glm::rotate(Mat4(1.0f), pitchAngle, right));
        cameraForward = glm::normalize(pitchRotation * cameraForward);
        cameraUp = glm::normalize(pitchRotation * cameraUp);
    }

    // Apply roll rotation around forward axis
    if (deltaRoll != 0.0f) {
        float rollAngle = deltaRoll * ROLL_SPEED;
        Mat3 rollRotation = Mat3(glm::rotate(Mat4(1.0f), rollAngle, cameraForward));
        cameraUp = glm::normalize(rollRotation * cameraUp);
    }

    RT_INFO("Forward: ({0}, {1}, {2})", cameraForward.x, cameraForward.y, cameraForward.z);

    SetCameraOrientation(cameraForward, cameraUp);
}

void UpdateCameraPosition(float deltaTime, float& deltaRoll) {
    Vec3 direction = VecUtils::Zero;

    if (Input::IsKeyPressed(Key::W) || Input::IsKeyPressed(Key::Up))
        direction += Vec3(uniformBufferData.u_CameraForward);
    if (Input::IsKeyPressed(Key::S) || Input::IsKeyPressed(Key::Down))
        direction -= Vec3(uniformBufferData.u_CameraForward);
    if (Input::IsKeyPressed(Key::A) || Input::IsKeyPressed(Key::Left))
        direction -= Vec3(uniformBufferData.u_CameraRight);
    if (Input::IsKeyPressed(Key::D) || Input::IsKeyPressed(Key::Right))
        direction += Vec3(uniformBufferData.u_CameraRight);
    if (Input::IsKeyPressed(Key::Space))
        direction += Vec3(uniformBufferData.u_CameraUp);
    if (Input::IsKeyPressed(Key::LeftShift))
        direction -= Vec3(uniformBufferData.u_CameraUp);

    // Roll controls
    deltaRoll = 0.0f;
    if (Input::IsKeyPressed(Key::Q))
        deltaRoll += deltaTime;
    if (Input::IsKeyPressed(Key::E))
        deltaRoll -= deltaTime;

    // FOV controls (via focus distance - larger = narrower FOV, smaller = wider FOV)
    const float FOV_SPEED = 1.0f;
    if (Input::IsKeyPressed(Key::R))
        uniformBufferData.u_FocusDistance += FOV_SPEED * deltaTime;
    if (Input::IsKeyPressed(Key::F))
        uniformBufferData.u_FocusDistance -= FOV_SPEED * deltaTime;

    // Clamp focus distance to reasonable values
    uniformBufferData.u_FocusDistance = glm::clamp(uniformBufferData.u_FocusDistance, 0.1f, 10.0f);

    const float CAMERA_SPEED = 5.0f;
    if (glm::length(direction) > 0.0f)
        direction = glm::normalize(direction) * deltaTime * CAMERA_SPEED;

    Vec3 cameraPosition = Vec3(uniformBufferData.u_CameraPosition);
    cameraPosition += direction;
    SetCameraPosition(cameraPosition);
}

void CameraUpdate(Scene& scene, float deltaTime) {
    if (Input::IsMouseButtonPressed(Mouse::ButtonRight)) {
        // default FPS camera behavior
        Input::SetCursorLocked(true);

        float deltaRoll = 0.0f;
        UpdateCameraPosition(deltaTime, deltaRoll);
        UpdateCameraDirection(-1 * Input::GetMouseDelta().x, -1 * Input::GetMouseDelta().y, deltaRoll);

        uniformBufferData.u_SampleIndex = 0; // reset accumulation on camera move
        uniformBufferData.u_Raybounces = 2;
        uniformBufferData.u_EnableGI = 0;
    } else {
        Input::SetCursorLocked(false);
        uniformBufferData.u_Raybounces = 4;
        uniformBufferData.u_EnableGI = Params::s_EnableGI ? 1 : 0;
    }
}