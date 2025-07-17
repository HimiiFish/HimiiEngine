#include "Engine.h"
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

    virtual void OnEvent(void *event) override
    {
        // 事件处理代码
    }

};

class TemplateProject : public Himii::Application {
public:
    TemplateProject()
    {
        // 初始化代码
        PushLayer(new ExampleLayer());
        PushLayer(new Himii::ImGuiLayer());
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
