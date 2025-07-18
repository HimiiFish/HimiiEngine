#pragma once
#include "Himii/Core/Layer.h"
#include "Himii/Events/Event.h"
#include "Himii/Events/MouseEvent.h"
#include "Himii/Events/KeyEvent.h"
#include "Himii/Events/ApplicationEvent.h"
namespace Himii
{
    class ImGuiLayer : public Layer 
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer()=default;

         void OnAttach();
         void OnDetach();
         void OnUpdate();
         void OnEvent(Event& event);

     private:
         bool OnMouseButtonPressed(MouseButtonPressedEvent &event);
         bool OnMouseButtonReleased(MouseButtonReleasedEvent &event);
         bool OnMouseMoved(MouseMovedEvent &event);
         bool OnMouseScrolled(MouseScrolledEvent &event);
         bool OnKeyPressed(KeyPressedEvent &event);
         bool OnKeyReleased(KeyReleasedEvent &event);
         //bool OnKeyTypedEvent(KeyTypedEvent &event);
         bool OnWindowResizeEvent(WindowResizeEvent &event);

    private:
         float m_Time = 0.0f;
    };
}
