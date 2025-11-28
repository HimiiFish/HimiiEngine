#include "EditorLayer.h"
#include "Engine.h"
#include "Himii/Core/EntryPoint.h"


#include <iostream>

namespace Himii
{
    class HimiiApp : public Application {
    public:
        HimiiApp(ApplicationCommandLineArgs args) : Application("Himii Editor", args)
        {
            PushOverlay(new EditorLayer());
        }

        virtual ~HimiiApp()
        {
            // 清理代码
        }
    };

    Application *CreateApplication(ApplicationCommandLineArgs args)
    {
        return new HimiiApp(args);
    }
} // namespace Himii
