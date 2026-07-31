#include "EditorLayer.h"
#include "Engine.h"
#include "EngineCore/Core/EntryPoint.h"

namespace Himii
{
    class HimiiApp : public Application {
    public:
        HimiiApp(ApplicationCommandLineArgs args) : Application("Himii Editor", args, CreateSplashWindowProps())
        {
            PushOverlay(new EditorLayer());
        }

        virtual ~HimiiApp()
        {
        }

    private:
        static WindowProps CreateSplashWindowProps()
        {
            WindowProps splash_window_props("Himii Editor", 640, 400);
            splash_window_props.Decorated = false;
            splash_window_props.Maximized = false;
            splash_window_props.CenterOnScreen = true;
            return splash_window_props;
        }
    };

    Application *CreateApplication(ApplicationCommandLineArgs args)
    {
        return new HimiiApp(args);
    }
} // namespace Himii
