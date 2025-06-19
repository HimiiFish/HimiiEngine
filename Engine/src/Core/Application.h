#pragma once

namespace Core
{
    class Application 
    {
    public:
        Application();
        virtual ~Application();

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