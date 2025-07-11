#pragma once
#include "Himii/Core/Layer.h"
#include "Himii/Events/ApplicationEvent.h"
#include "Himii/Events/MouseEvent.h"
#include "Himii/Events/KeyEvent.h"

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
         float m_Time = 0.0f;
    };
}
