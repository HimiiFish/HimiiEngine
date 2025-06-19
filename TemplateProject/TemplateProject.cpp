#include "Engine.h"

class TemplateProject : public Engine::Application {
public:
    TemplateProject()
    {
        // 初始化代码
    }

    virtual ~TemplateProject()
    {
        // 清理代码
    }
};

Engine::Application *Engine::CreateApplication()
{
    return new TemplateProject();
}
