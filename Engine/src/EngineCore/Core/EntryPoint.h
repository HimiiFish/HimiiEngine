#pragma once
#include "EngineCore/Core/Application.h"
#include "EngineCore/Core/Core.h"

extern Himii::Application* Himii::CreateApplication(ApplicationCommandLineArgs args);

// -------------------------------------------------------------------------
// 应用入口（Debug 为控制台子系统；Release 链接为 WINDOWS 子系统 + mainCRTStartup）
// -------------------------------------------------------------------------
int main(int argc, char** argv)
{
    Himii::Log::Init();

    HIMII_PROFILE_BEGIN_SESSION("Startup", "HimiiProfile-Startup.json");
    auto app = Himii::CreateApplication({ argc, argv });
    HIMII_PROFILE_END_SESSION();

    HIMII_PROFILE_BEGIN_SESSION("Runtime", "HimiiProfile-Runtime.json");
    app->Run();
    HIMII_PROFILE_END_SESSION();

    HIMII_PROFILE_BEGIN_SESSION("Shutdown", "HimiiProfile-Shutdown.json");
    delete app;
    HIMII_PROFILE_END_SESSION();

    return 0;
}


