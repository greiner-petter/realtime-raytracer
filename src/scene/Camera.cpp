#include "scene/Camera.h"
#include "common/Input.h"
#include "common/Log.h"
#include "vulkan/VulkanAPI.h"
#include "scene/Scene.h"
#include <glm/glm.hpp>

extern UBO uniformBufferData;

float yaw = 3.1415f;   // rotation around Y axis
float pitch = 0.0f; // rotation around X axis (camera right)

void UpdateCameraDirection(float deltaX, float deltaY) {
    float sensitivity = 0.005f;
    yaw -= deltaX * sensitivity;
    pitch -= deltaY * sensitivity;

    // Clamp pitch to avoid looking straight up/down
    const float pitchLimit = glm::radians(89.0f);
    if (pitch > pitchLimit) pitch = pitchLimit;
    if (pitch < -pitchLimit) pitch = -pitchLimit;

    glm::vec3 forward;
    forward.x = cos(pitch) * sin(yaw);
    forward.y = sin(pitch);
    forward.z = cos(pitch) * cos(yaw);
    forward = glm::normalize(forward);

    glm::vec3 cameraForward = forward;
    RT_INFO("Camera Forward: ({0}, {1}, {2})", cameraForward.x, cameraForward.y, cameraForward.z);

    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraForward, worldUp));
    glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraForward));

    uniformBufferData.u_CameraForward = glm::vec4(cameraForward, 0.0f);
    uniformBufferData.u_CameraRight = glm::vec4(cameraRight, 0.0f);
    uniformBufferData.u_CameraUp = glm::vec4(cameraUp, 0.0f);
}

void UpdateCameraPosition(float deltaTime) {
    glm::vec3 direction = glm::vec3(0.0f);

    if (Input::IsKeyPressed(Key::W) || Input::IsKeyPressed(Key::Up))
        direction += glm::vec3(uniformBufferData.u_CameraForward);
    if (Input::IsKeyPressed(Key::S) || Input::IsKeyPressed(Key::Down))
        direction -= glm::vec3(uniformBufferData.u_CameraForward);
    if (Input::IsKeyPressed(Key::A) || Input::IsKeyPressed(Key::Left))
        direction -= glm::vec3(uniformBufferData.u_CameraRight);
    if (Input::IsKeyPressed(Key::D) || Input::IsKeyPressed(Key::Right))
        direction += glm::vec3(uniformBufferData.u_CameraRight);
    if (Input::IsKeyPressed(Key::E))
        direction += glm::vec3(uniformBufferData.u_CameraUp);
    if (Input::IsKeyPressed(Key::Q))
        direction -= glm::vec3(uniformBufferData.u_CameraUp);

    const float CAMERA_SPEED = 5.0f;
    if (glm::length(direction) > 0.0f)
        direction = glm::normalize(direction) * deltaTime * CAMERA_SPEED;

    glm::vec3 cameraPosition = glm::vec3(uniformBufferData.u_CameraPosition);
    cameraPosition += direction;
    uniformBufferData.u_CameraPosition = glm::vec4(cameraPosition, 0.0f);
}

void CameraUpdate(Scene& scene, float deltaTime) {
    if (Input::IsMouseButtonPressed(Mouse::ButtonRight)) {
        // default FPS camera behavior
        Input::SetCursorLocked(true);
        UpdateCameraDirection(Input::GetMouseDelta().x, Input::GetMouseDelta().y);
        UpdateCameraPosition(deltaTime);
    } else {
        Input::SetCursorLocked(false);
    }
}