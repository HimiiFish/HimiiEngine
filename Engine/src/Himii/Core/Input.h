#pragma once
#include "Hepch.h"
#include "glm/glm.hpp"

namespace Himii
{
    class Input {
    public:
        static bool IsKeyPressed(int key);
        static bool IsMouseButtonPressed(int button);
        static glm::vec2 GetMousePosition();
        static float GetMouseX();
        static float GetMouseY();
    };
} // namespace Core
