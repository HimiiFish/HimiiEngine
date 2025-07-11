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
