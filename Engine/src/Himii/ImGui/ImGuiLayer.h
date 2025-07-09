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

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnEvent() override;

    private:
    };
}
