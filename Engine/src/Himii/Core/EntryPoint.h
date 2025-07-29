#pragma once
#include "Himii/Core/Application.h"
#include "Log.h"

extern Himii::Application *Himii::CreateApplication();

int main(int argc, char *argv[])
{
    Himii::Log::Init();
    HIMII_CORE_INFO("Application starting...");
    //LOG_WARNING("This is a warning message.");
    // 创建应用程序实例
    auto *app = Himii::CreateApplication();
   
    // 初始化应用程序
    //app->Initialize();

    // 运行应用程序
    app->Run();

    // 清理资源
    delete app;

    return 0;
}
