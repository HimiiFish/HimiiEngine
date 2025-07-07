#include "Engine.h"
#include <iostream>

class ExampleLayer : public Himii::Layer
{
public:
    ExampleLayer() : Layer("ExampleLayer")
    {
        // 初始化代码
        LOG_WARNING("ExampleLayer initialized");
    }

    virtual void OnUpdate() override
    {
        // 更新时的代码
        LOG_INFO("ExampleLayer is updating");
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
