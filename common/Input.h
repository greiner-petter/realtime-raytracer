#pragma once

#include "common/KeyCodes.h"
#include "common/MouseCodes.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

class Input {
public:
    static bool IsKeyPressed(const KeyCode key);
    static bool IsMouseButtonPressed(const MouseCode button);
    static glm::vec2 GetMousePosition();
    static glm::vec2 GetMouseDelta();
    static void SetCursorLocked(bool InLocked);

    static void Tick();
};