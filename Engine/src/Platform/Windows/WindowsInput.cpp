#include "Hepch.h"
#include "Himii/Core/Input.h"
#include "Himii/Core/Application.h"
#include "GLFW/glfw3.h"

#include <unordered_map>

namespace Himii
{
    namespace
    {
        struct KeyFrameState
        {
            bool WasDownLastFrame = false;
            bool IsDownThisFrame = false;
        };

        std::unordered_map<KeyCode, KeyFrameState> s_KeyFrameStates;

        bool QueryKeyDownState(KeyCode key)
        {
            auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
            if (!window)
                return false;
            const int state = glfwGetKey(window, static_cast<int>(key));
            return state == GLFW_PRESS || state == GLFW_REPEAT;
        }

        KeyFrameState& GetKeyFrameState(KeyCode key)
        {
            return s_KeyFrameStates[key];
        }
    }

    bool Input::IsKeyPressed(const KeyCode key)
    {
        const bool isDown = QueryKeyDownState(key);
        GetKeyFrameState(key).IsDownThisFrame = isDown;
        return isDown;
    }

    bool Input::IsKeyJustPressed(const KeyCode key)
    {
        const bool isDown = QueryKeyDownState(key);
        KeyFrameState& keyFrameState = GetKeyFrameState(key);
        const bool wasDownLastFrame = keyFrameState.WasDownLastFrame;
        keyFrameState.IsDownThisFrame = isDown;
        return isDown && !wasDownLastFrame;
    }

    bool Input::IsKeyJustReleased(const KeyCode key)
    {
        const bool isDown = QueryKeyDownState(key);
        KeyFrameState& keyFrameState = GetKeyFrameState(key);
        const bool wasDownLastFrame = keyFrameState.WasDownLastFrame;
        keyFrameState.IsDownThisFrame = isDown;
        return !isDown && wasDownLastFrame;
    }

    bool Input::IsMouseButtonPressed(const MouseCode button)
    {
        auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        if (!window)
            return false;
        const int state = glfwGetMouseButton(window, static_cast<int>(button));
        return state == GLFW_PRESS;
    }

    glm::vec2 Input::GetMousePosition()
    {
        auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        if (!window)
            return glm::vec2(0.0f);
        double positionX = 0.0;
        double positionY = 0.0;
        glfwGetCursorPos(window, &positionX, &positionY);
        return glm::vec2(static_cast<float>(positionX), static_cast<float>(positionY));
    }

    float Input::GetMouseX()
    {
        return GetMousePosition().x;
    }

    float Input::GetMouseY()
    {
        return GetMousePosition().y;
    }

    float Input::GetAxisHorizontal()
    {
        float axis = 0.0f;
        if (IsKeyPressed(Key::A) || IsKeyPressed(Key::Left))
            axis -= 1.0f;
        if (IsKeyPressed(Key::D) || IsKeyPressed(Key::Right))
            axis += 1.0f;
        return axis;
    }

    float Input::GetAxisVertical()
    {
        float axis = 0.0f;
        if (IsKeyPressed(Key::S) || IsKeyPressed(Key::Down))
            axis -= 1.0f;
        if (IsKeyPressed(Key::W) || IsKeyPressed(Key::Up))
            axis += 1.0f;
        return axis;
    }

    void Input::EndFrame()
    {
        // 先刷新本帧曾出现过的键的物理状态，再写入 WasDown，供下一帧边沿检测。
        for (auto& [keyCode, keyFrameState] : s_KeyFrameStates)
        {
            keyFrameState.IsDownThisFrame = QueryKeyDownState(keyCode);
            keyFrameState.WasDownLastFrame = keyFrameState.IsDownThisFrame;
        }
    }
}
