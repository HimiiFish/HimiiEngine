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

         virtual void OnAttach() override;
         virtual void OnDetach() override;
         virtual void OnImGuiRender() override;
         virtual void OnEvent(Event& e) override;

         void Begin();
         void End();

         void BlockEvents(bool block)
         {
             m_BlockEvents = block;
         }
         void SetDarkThemeColors();
         uint32_t GetActiveWidgetID() const;

    private:
         bool m_BlockEvents = true;
         std::string m_IniFilePath;
    };
}
