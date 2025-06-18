#pragma once

namespace Core
{
    class Application 
    {
    public:
        Application();
        virtual ~Application();

        // 启动应用程序
        void Run();

        inline static Application &Get()
        {
            return *s_Instance;
        }

    private:

        bool m_Running = true;

        static Application *s_Instance;
    };
    Application* CreateApplication();
}