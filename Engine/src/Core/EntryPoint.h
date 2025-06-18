#pragma once

int main(int argc, char *argv[])
{
    // 创建应用程序实例
    auto *app = Core::CreateApplication();

    // 初始化应用程序
    app->Initialize();

    // 运行应用程序
    app->Run();

    // 清理资源
    delete app;

    return 0;
}
