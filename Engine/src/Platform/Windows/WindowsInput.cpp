//#include "Hepch.h"
//#include "Himii/Core/Input.h"
//#include "Himii/Core/Application.h"
//#include "GLFW/glfw3.h"
//
//
//namespace Himii
//{
//    /*bool Input::IsKeyPressed(int key)
//    {
//        auto *window = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow());
//        int state = glfwGetKey(window, key);
//        return state == GLFW_PRESS || state == GLFW_REPEAT;
//    }
//
//    bool Input::IsMouseButtonPressed(int button)
//    {
//        auto *window = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow());
//        int state = glfwGetMouseButton(window, button);
//        return state == GLFW_PRESS;
//    }
//
//    glm::vec2 Input::GetMousePosition()
//    {
//        auto *window = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow());
//        double x, y;
//        glfwGetCursorPos(window, &x, &y);
//        return glm::vec2(static_cast<float>(x), static_cast<float>(y));
//    }*/
//
//    float Input::GetMouseX()
//    {
//        return GetMousePosition().x;
//    }
//
//    float Input::GetMouseY()
//    {
//        return GetMousePosition().y;
//    }
//}