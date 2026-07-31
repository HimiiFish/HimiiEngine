#pragma once
#include "EngineCore/Core/Layer.h"
#include "EngineCore/Events/Event.h"
#include "EngineCore/Events/MouseEvent.h"
#include "EngineCore/Events/KeyEvent.h"
#include "EngineCore/Events/ApplicationEvent.h"
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
         void ApplyEditorTheme();
         void LoadEditorFonts();
         void EnableLayoutPersistence();
         uint32_t GetActiveWidgetID() const;

    private:
         bool m_BlockEvents = true;
         bool m_LayoutPersistenceEnabled = false;
         std::string m_IniFilePath;
    };
}
