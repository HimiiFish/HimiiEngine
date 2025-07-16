#pragma once
#include "Himii/Core/Layer.h"
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
