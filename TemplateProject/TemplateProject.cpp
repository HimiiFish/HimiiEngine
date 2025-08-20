#include "Engine.h"
#include "Himii/Core/EntryPoint.h"
#include "ExampleLayer.h"
#include "Example2D.h"
#include "CubeLayer.h"

#include <iostream>


class TemplateProject : public Himii::Application {
public:
    TemplateProject()
    {
        // 初始化代码
        //PushLayer(new ExampleLayer());
        //PushLayer(new Example2D);
        PushLayer(new CubeLayer); // 启用立方体示例
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
