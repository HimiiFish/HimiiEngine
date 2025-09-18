#include "MainScene.h"
#include "Engine.h"
#include "Himii/Core/EntryPoint.h"

#include <iostream>

namespace Himii
{
    class HimiiProject : public Application {
    public:
        HimiiProject()
        {
            PushOverlay(new MainScene());
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
