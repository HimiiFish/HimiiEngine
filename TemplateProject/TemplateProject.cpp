#include "Engine.h"
#include "Himii/Core/EntryPoint.h"
#include "ExampleLayer.h"
#include "Example2D.h"
#include <iostream>


class TemplateProject : public Himii::Application {
public:
    TemplateProject()
    {
        // 初始化代码
        //PushLayer(new ExampleLayer());
        PushLayer(new Example2D);
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
