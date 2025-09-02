#pragma once
#include "Himii/Core/Application.h"
#include "Log.h"

extern Himii::Application *Himii::CreateApplication();

int main(int argc, char *argv[])
{
    Himii::Log::Init();

    // 创建应用程序实例
    //HIMII_PROFILE_BEGIN_SESSION("Startup", "HimiiEngine_Profile-Startup.json");
    auto *app = Himii::CreateApplication();
    HIMII_PROFILE_END_SESSION();
   
    // 初始化应用程序
    //app->Initialize();

    // 运行应用程序
   //HIMII_PROFILE_BEGIN_SESSION("Runtime", "HimiiEngine_Profile-Runtime.json");
    app->Run();
    //HIMII_PROFILE_END_SESSION();

    // 清理资源
    //HIMII_PROFILE_BEGIN_SESSION("Shutdown", "HimiiEngine_Profile-Shutdown.json");
    delete app;
    //HIMII_PROFILE_END_SESSION();

    return 0;
}
