#include "Engine.h"
#include "Himii/Core/EntryPoint.h"
#include "EditorLayer.h"

#include <iostream>

namespace Himii
{
    class HimiiApp : public Application {
    public:
        HimiiApp()
        {
            PushOverlay(new EditorLayer());
        }

        virtual ~HimiiApp()
        {
            // 清理代码
        }
    };

   Application *CreateApplication()
    {
        return new HimiiApp();
    }
}
