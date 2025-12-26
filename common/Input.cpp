#include "common/Input.h"

#include "common/Window.h"
#include "GLFW/glfw3.h"

static Vector2d s_LastMousePosition;
static Vector2d s_MouseDelta;

bool Input::IsKeyPressed(const KeyCode key) {
    auto* window = Window::GetGLFWwindow();
    auto state = glfwGetKey(window, static_cast<int32_t>(key));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(const MouseCode button) {
    auto* window = Window::GetGLFWwindow();
    auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
    return state == GLFW_PRESS;
}

Vector2d Input::GetMousePosition() {
    auto* window = Window::GetGLFWwindow();
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    return Vector2d((float)xpos, (float)ypos);
}
Vector2d Input::GetMouseDelta() {
    return s_MouseDelta;
}

void Input::SetCursorLocked(bool InLocked) {
    auto* window = Window::GetGLFWwindow();
    bool needsReset = GLFW_CURSOR_NORMAL == glfwGetInputMode(window, GLFW_CURSOR) && InLocked;
    glfwSetInputMode(window, GLFW_CURSOR, InLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    if (needsReset) {
        s_MouseDelta = Vector2d();
        s_LastMousePosition = GetMousePosition();
    }
}

void Input::Tick() {
    s_MouseDelta = GetMousePosition() - s_LastMousePosition;
    s_LastMousePosition = GetMousePosition();
}