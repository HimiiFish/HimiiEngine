#include "Engine.h"
#include "Himii/Core/EntryPoint.h"

#include "MainScene.h"
#include "UILayer.h"

#include <iostream>

namespace Himii
{
    class HimiiProject : public Application {
    public:
        HimiiProject()
        {
            PushLayer(new MainScene());
            PushOverlay(new UILayer());
        }

        virtual ~HimiiProject()
        {
            // «Â¿Ì¥˙¬Î
        }
    };

    Application *CreateApplication()
    {
        return new HimiiProject();
    }
} // namespace Himii
