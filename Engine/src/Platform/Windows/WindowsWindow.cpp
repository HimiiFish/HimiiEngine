#include "WindowsWindow.h"

#include <cassert>

#include "Himii/Events/ApplicationEvent.h"
#include "Himii/Events/KeyEvent.h"
#include "Himii/Events/MouseEvent.h"
#include "Himii/Core/Assert.h"
#include <glad/glad.h>

namespace Himii
{

    static bool s_SDLInitialized = false;

    WindowsWindow::WindowsWindow(const WindowProps &props) : m_Window(nullptr), m_Context(nullptr)
    {
        WindowsWindow::Init(props);
    }

    WindowsWindow::~WindowsWindow()
    {
        WindowsWindow::Shutdown();
    }

    void WindowsWindow::Init(const WindowProps &props)
    {
        SDL_PropertiesID creatProps = SDL_CreateProperties();
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            //std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            LOG_CORE_ERROR("SDL_Init failed: {0}", SDL_GetError());
            return;
        }
        //设置标题
        m_Data.Title = props.Title;
        //SDL_SetStringProperty(creatProps, SDL_PROP_WINDOW_CREATE_TITLE_STRING, m_Data.Title.c_str());
        //设置宽度和高度
        m_Data.Width = props.Width;
        //SDL_SetNumberProperty(creatProps,SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, m_Data.Width);
        m_Data.Height = props.Height;
        //SDL_SetNumberProperty(creatProps, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, m_Data.Height);
        //设置vsync
        m_Data.VSync = true; // 默认开启VSync
        //设置窗口标志
        //SDL_SetNumberProperty(creatProps, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER,
                              //SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        LOG_CORE_INFO("Creating window {0} ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);
        /*if (!s_SDLInitialized)
        {
            int success = SDL_Init(SDL_INIT_VIDEO);
            LOG_CORE_ERROR_F("SDL_Init result: {0}", success);
            s_SDLInitialized = true;
        }*/
        // Set OpenGL attributes
        /*SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);*/

        m_Window = SDL_CreateWindow(m_Data.Title.c_str(), m_Data.Width,
                                    m_Data.Height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        assert(m_Window);

        m_Context = SDL_GL_CreateContext(m_Window);
        assert(m_Context);

        SDL_GL_MakeCurrent(m_Window, m_Context);

        // Initialize GLAD
        int gladStatus = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

        SDL_Renderer *renderer = SDL_CreateRenderer(m_Window, nullptr);
        if (!renderer)
        {
            SDL_Log("Create renderer failed: %s", SDL_GetError());
            return;
        }
        SDL_SetRenderDrawColor(renderer, 80, 18, 16, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        // Set VSync
        SetVSync(m_Data.VSync);

        //设置窗口回调
        
    }

    void WindowsWindow::Shutdown()
    {
        if (m_Context)
        {
            SDL_GL_DestroyContext(m_Context);
            m_Context = nullptr;
        }
        if (m_Window)
        {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
        SDL_DestroyWindow(m_Window);
    }

    void WindowsWindow::Update()
    {
        //LOG_CORE_INFO_F(" Updating window{0}({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);
        SDL_Event event;
        SDL_PollEvent(&event);
        switch (event.type)
        {
            case SDL_EVENT_WINDOW_RESIZED:
            {
                m_Data.Width = event.window.data1;
                m_Data.Height = event.window.data2;
                WindowResizeEvent resizeEvent(m_Data.Width, m_Data.Height);
                m_Data.EventCallback(resizeEvent);
                break;
            }
            case SDL_EVENT_QUIT:
            {
                WindowCloseEvent closeEvent;
                m_Data.EventCallback(closeEvent);
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            {
                KeyPressedEvent keyPressedEvent(event.key.scancode);
                m_Data.EventCallback(keyPressedEvent);
                break;
            }
            default:
                break;
        }
        if (event.type == SDL_EVENT_QUIT)
        {
            WindowCloseEvent closeEvent;
            m_Data.EventCallback(closeEvent);
        }


    }

    uint32_t WindowsWindow::GetWidth() const
    {
        return m_Data.Width;
    }

    uint32_t WindowsWindow::GetHeight() const
    {
        return m_Data.Height;
    }

    void WindowsWindow::SetEventCallback(const EventCallbackFn &callback)
    {
        m_Data.EventCallback = callback;
    }

    void WindowsWindow::SetVSync(bool enabled)
    {
        if (enabled)
        {
            SDL_GL_SetSwapInterval(1);
        }
        else
        {
            SDL_GL_SetSwapInterval(0);
        }
        m_Data.VSync = enabled;
    }

    bool WindowsWindow::IsVSync() const
    {
        return m_Data.VSync;
    }

    void *WindowsWindow::GetNativeWindow() const
    {
        return m_Window;
    }

}
