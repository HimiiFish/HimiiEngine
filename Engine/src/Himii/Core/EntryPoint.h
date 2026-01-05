#pragma once
#include "Himii/Core/Application.h"
#include "Himii/Core/Core.h"

#ifdef HIMII_PLATFORM_WINDOWS

extern Himii::Application *Himii::CreateApplication(ApplicationCommandLineArgs args);

// -------------------------------------------------------------------------
// 现有的 main 函数 (保持不变)
// -------------------------------------------------------------------------
int main(int argc, char **argv)
{
    Himii::Log::Init();

    HIMII_PROFILE_BEGIN_SESSION("Startup", "HimiiProfile-Startup.json");
    auto app = Himii::CreateApplication({argc, argv});
    HIMII_PROFILE_END_SESSION();

    HIMII_PROFILE_BEGIN_SESSION("Runtime", "HimiiProfile-Runtime.json");
    app->Run();
    HIMII_PROFILE_END_SESSION();

    HIMII_PROFILE_BEGIN_SESSION("Shutdown", "HimiiProfile-Shutdown.json");
    delete app;
    HIMII_PROFILE_END_SESSION();

    return 0;
}

// Todo: 暂时注释掉 WinMain，方便出现问题时调试

//// -------------------------------------------------------------------------
//// [新增] WinMain 函数 (专门为了解决 WIN32 模式下的入口问题)
//// -------------------------------------------------------------------------
//#include <Windows.h>
//
//int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
//{
//    return main(__argc, __argv);
//}

#endif
