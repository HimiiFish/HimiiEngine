#pragma once
#include "Himii/Core/KeyCodes.h"
#include "Himii/Core/MouseCodes.h"
#include "glm/glm.hpp"

namespace Himii
{
    class Input {
    public:
        static bool IsKeyPressed(KeyCode key);
        static bool IsMouseButtonPressed(MouseCode button);
        static glm::vec2 GetMousePosition();
        static float GetMouseX();
        static float GetMouseY();
    };
} // namespace Core
