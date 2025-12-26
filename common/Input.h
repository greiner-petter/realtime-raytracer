#pragma once

#include "common/KeyCodes.h"
#include "common/MouseCodes.h"
#include "common/vector2d.h"

class Input {
public:
    static bool IsKeyPressed(const KeyCode key);
    static bool IsMouseButtonPressed(const MouseCode button);
    static Vector2d GetMousePosition();
    static Vector2d GetMouseDelta();
    static void SetCursorLocked(bool InLocked);

    static void Tick();
};