#include "WindowsWindow.h"
#include "Hepch.h"

#include "Himii/Core/Input.h"
#include "Himii/Core/Log.h"

#include "Himii/Events/ApplicationEvent.h"
#include "Himii/Events/KeyEvent.h"
#include "Himii/Events/MouseEvent.h"

namespace Engine
{
    static uint8_t s_SDLWindowCout = 0;

    static void SDLErrorCallback(int error, const char *description)
    {
        LOG_CORE_ERROR("SDL Error ({0}):{1}}", error, description);
    }

    WindowsWindow::WindowsWindow(const WindowProps &props)
    {
        Init(props);
    }
    WindowsWindow::~WindowsWindow()
    {
        Shutdown();
    }
    void WindowsWindow::Init(const WindowProps &props)
    {
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        LOG_CORE_INFO("Create Window {0}{1}{2}", props.Title, props.Width, props.Height);
        if (s_SDLWindowCout == 0)
        {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return;
        }

        m_Window = SDL_CreateWindow(props.Title, props.Width, props.Height, SDL_WINDOW_OPENGL);
        ++s_SDLWindowCout;
    }

    void WindowsWindow::Shutdown()
    {
        SDL_DestroyWindow(m_Window);
        --s_SDLWindowCout;

        if (s_SDLWindowCout == 0)
        {
            SDL_Quit();
        }
    }

    void WindowsWindow::OnUpdate()
    {
        SDL_Event event;
        SDL_PollEvent(&event);
    }

    void WindowsWindow::SetVSync(bool enabled)
    {
        if (enabled)
            SDL_GL_SetSwapInterval(1);
        else
            SDL_GL_SetSwapInterval(0);
        m_Data.VSync = enabled;
    }

    bool WindowsWindow::IsVSync() const
    {
        return m_Data.VSync;
    }
} // namespace Engine
