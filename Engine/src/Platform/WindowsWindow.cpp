#include "WindowsWindow.h"

#include <cassert>

#include "Himii/Events/ApplicationEvent.h"
#include "Himii/Events/KeyEvent.h"
#include "Himii/Events/MouseEvent.h"
#include "Himii/Core/Log.h"
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

        //设置标题
        m_Data.Title = props.Title;
        SDL_SetStringProperty(creatProps, SDL_PROP_WINDOW_CREATE_TITLE_STRING, m_Data.Title.c_str());
        //设置宽度和高度
        m_Data.Width = props.Width;
        SDL_SetNumberProperty(creatProps,SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, m_Data.Width);
        m_Data.Height = props.Height;
        SDL_SetNumberProperty(creatProps, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, m_Data.Height);
        //设置vsync
        m_Data.VSync = true; // 默认开启VSync
        //设置窗口标志
        SDL_SetNumberProperty(creatProps, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        LOG_CORE_INFO("Creating window)");

        if (!s_SDLInitialized)
        {
            int success = SDL_Init(SDL_INIT_VIDEO);
            assert(success == 0);
            s_SDLInitialized = true;
        }

        // Set OpenGL attributes
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        m_Window = SDL_CreateWindowWithProperties(creatProps);

        assert(m_Window);

        m_Context = SDL_GL_CreateContext(m_Window);
        assert(m_Context);

        SDL_GL_MakeCurrent(m_Window, m_Context);

        // Initialize GLAD
        int gladStatus = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
        assert(gladStatus);


        // Set VSync
        SetVSync(m_Data.VSync);

        //销毁 SDL_PropertiesID
        SDL_DestroyProperties(creatProps);
        callback
        
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
    }

    void WindowsWindow::Update()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                {
                    WindowCloseEvent closeEvent;
                    m_Data.EventCallback(closeEvent);
                    break;
                }
                case SDL_EVENT_WINDOW_RESIZED:
                {
                    m_Data.Width = event.window.data1;
                    m_Data.Height = event.window.data2;
                    WindowResizeEvent resizeEvent(m_Data.Width, m_Data.Height);
                    m_Data.EventCallback(resizeEvent);
                    break;
                }
                case SDL_EVENT_KEY_DOWN:
                {
                    KeyPressedEvent keyEvent(event.key.scancode);
                    m_Data.EventCallback(keyEvent);
                    break;
                }
                case SDL_EVENT_KEY_UP:
                {
                    KeyReleasedEvent keyEvent(event.key.scancode);
                    m_Data.EventCallback(keyEvent);
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                {
                    MouseButtonPressedEvent mouseEvent(event.button.button);
                    m_Data.EventCallback(mouseEvent);
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP:
                {
                    MouseButtonReleasedEvent mouseEvent(event.button.button);
                    m_Data.EventCallback(mouseEvent);
                    break;
                }
                case SDL_EVENT_MOUSE_MOTION:
                {
                    MouseMovedEvent mouseEvent((float)event.motion.x, (float)event.motion.y);
                    m_Data.EventCallback(mouseEvent);
                    break;
                }
                case SDL_EVENT_MOUSE_WHEEL:
                {
                    MouseScrolledEvent scrollEvent((float)event.wheel.x, (float)event.wheel.y);
                    m_Data.EventCallback(scrollEvent);
                    break;
                }
                default:
                    LOG_CORE_WARNING("Unhandled event type {0}");
                    break;
            }
        }

        SDL_GL_SwapWindow(m_Window);
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
