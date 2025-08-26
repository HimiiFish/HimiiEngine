#include "Engine.h"
using namespace Himii;
#include "EditorLayer.h"
#include "Example2D.h"
#include "ExampleLayer.h"
#include "Himii/Core/EntryPoint.h"

#include <iostream>

class TemplateProject : public Application {
public:
    TemplateProject()
    {
        // 初始化代码
        // PushLayer(new ExampleLayer());
        PushLayer(new Example2D);
        // 叠加编辑器Overlay
        //PushOverlay(new EditorLayer());
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
