#pragma once
#include "EngineCore/Core/KeyCodes.h"
#include "EngineCore/Core/MouseCodes.h"
#include "glm/glm.hpp"

namespace Himii
{
    class Input {
    public:
        static bool IsKeyPressed(KeyCode key);
        static bool IsKeyJustPressed(KeyCode key);
        static bool IsKeyJustReleased(KeyCode key);
        static bool IsMouseButtonPressed(MouseCode button);
        static glm::vec2 GetMousePosition();
        static float GetMouseX();
        static float GetMouseY();
        static float GetAxisHorizontal();
        static float GetAxisVertical();

        /// 每帧主循环末尾调用，刷新按键边沿检测状态。
        static void EndFrame();
    };
} // namespace Himii
