#include "Engine.h"
#include "imgui.h"
#include <iostream>

class ExampleLayer : public Himii::Layer
{
public:
    ExampleLayer() : Layer("ExampleLayer")
    {
    }

    virtual void OnUpdate() override
    {
        if (Himii::Input::IsKeyPressed(Himii::Key::Space))
        {
            HIMII_INFO("Space key is pressed!");
        }
    }
    virtual void OnImGuiRender() override
    {
        ImGui::Text("Hello, this is an example layer!");
        if (ImGui::Button("Click Me"))
        {
            HIMII_INFO("Button clicked!");
        }
    }

    virtual void OnEvent(Himii::Event& event ) override
    {
        // 事件处理代码
        if (event.GetEventType() == Himii::EventType::KeyPressed)
        {
            Himii::KeyPressedEvent &keyEvent = static_cast<Himii::KeyPressedEvent &>(event);
            if (keyEvent.GetKeyCode() == Himii::Key::Tab)
            {
                HIMII_INFO_F("Tab key pressed");
            }
            HIMII_INFO_F("Key Pressed: {0}", (char)keyEvent.GetKeyCode());
        }
    }

};

class TemplateProject : public Himii::Application {
public:
    TemplateProject()
    {
        // 初始化代码
        PushLayer(new ExampleLayer());
    }

    virtual ~TemplateProject()
    {
        // 清理代码
    }
};

Himii::Application *Himii::CreateApplication()
{
    return new TemplateProject();
}
