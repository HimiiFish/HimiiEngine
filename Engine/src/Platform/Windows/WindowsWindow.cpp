#include "WindowsWindow.h"
#include "Hepch.h"
#include "EngineCore/Core/Input.h"
#include "EngineCore/Core/FileSystem.h"
#include "EngineCore/Core/Log.h"
#include "Module/Render/RHI/RHI.h"

#include "EngineCore/Events/ApplicationEvent.h"
#include "EngineCore/Events/KeyEvent.h"
#include "EngineCore/Events/MouseEvent.h"

// Windows 平台下，用 Win32 API 为 GLFW 创建的窗口设置图标
#include <GLFW/glfw3.h>
#ifdef HIMII_PLATFORM_WINDOWS
#include <windows.h>
#include <dwmapi.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#pragma comment(lib, "dwmapi.lib")
#endif
namespace Himii
{
    static uint8_t s_GLFWWindwCount = 0;

#ifdef HIMII_PLATFORM_WINDOWS
    static void ApplyBorderlessWindow(HWND window_handle)
    {
        if (!window_handle)
            return;

        LONG_PTR window_style = GetWindowLongPtr(window_handle, GWL_STYLE);
        window_style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        SetWindowLongPtr(window_handle, GWL_STYLE, window_style);

        LONG_PTR extended_style = GetWindowLongPtr(window_handle, GWL_EXSTYLE);
        extended_style &= ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE);
        SetWindowLongPtr(window_handle, GWL_EXSTYLE, extended_style);

        SetWindowPos(window_handle, nullptr, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    static void ApplyDarkNativeTitleBar(HWND windowHandle)
    {
        if (!windowHandle)
            return;

        constexpr DWORD useImmersiveDarkMode = 20;
        BOOL useDarkMode = TRUE;
        DwmSetWindowAttribute(windowHandle, useImmersiveDarkMode, &useDarkMode, sizeof(useDarkMode));

        constexpr DWORD captionColorAttribute = 35;
        constexpr DWORD captionTextColorAttribute = 36;
        const COLORREF captionBackground = RGB(31, 31, 31);
        const COLORREF captionText = RGB(230, 230, 230);
        DwmSetWindowAttribute(windowHandle, captionColorAttribute, &captionBackground, sizeof(captionBackground));
        DwmSetWindowAttribute(windowHandle, captionTextColorAttribute, &captionText, sizeof(captionText));
    }
#endif

    static void GLFWErrorCallback(int error, const char *description)
    {
        HIMII_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
    }

    WindowsWindow::WindowsWindow(const WindowProps &props)
    {
        HIMII_PROFILE_FUNCTION();

        Init(props);
    }

    WindowsWindow::~WindowsWindow()
    {
        HIMII_PROFILE_FUNCTION();

        Shutdown();
    }

    void WindowsWindow::Init(const WindowProps &props)
    {
        HIMII_PROFILE_FUNCTION();

        // 设置标题
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        HIMII_CORE_INFO("Creating window {0} ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);

        if (s_GLFWWindwCount == 0)
        {
            HIMII_PROFILE_SCOPE("glfwInit");
            int sucess = glfwInit();
            HIMII_CORE_ASSERT(sucess, "Failed to initialize GLFW");
            glfwSetErrorCallback(GLFWErrorCallback);
        }

        HIMII_PROFILE_SCOPE("glfwCreateWindow");

        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_DECORATED, props.Decorated ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_MAXIMIZED, props.Maximized ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

        m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
        ++s_GLFWWindwCount;

        if (!m_Window)
        {
            HIMII_CORE_ERROR("Failed to create GLFW window");
            glfwTerminate();
            return;
        }

        m_Context = RHI::CreateGraphicsContext(m_Window);
        m_Context->Init();

#ifdef HIMII_PLATFORM_WINDOWS
        {
            HWND window_handle = glfwGetWin32Window(m_Window);
            if (window_handle)
            {
                const std::filesystem::path iconPath =
                        FileSystem::MaterializeLooseFile("resources/icons/HimiiEngine.ico");
                HICON window_icon = static_cast<HICON>(LoadImageA(nullptr, iconPath.string().c_str(), IMAGE_ICON, 0, 0,
                                                                LR_LOADFROMFILE | LR_DEFAULTSIZE));

                if (window_icon)
                {
                    SendMessage(window_handle, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(window_icon));
                    SendMessage(window_handle, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(window_icon));
                }
                else
                {
                    HIMII_CORE_WARNING("Failed to load window icon from path: {0}", iconPath.string());
                }

                if (props.Decorated)
                    ApplyDarkNativeTitleBar(window_handle);
                else
                    ApplyBorderlessWindow(window_handle);
            }
        }
#endif

        if (props.Maximized)
            glfwMaximizeWindow(m_Window);

        if (props.CenterOnScreen)
            CenterOnScreen();

        {
            int windowWidth = 0;
            int windowHeight = 0;
            glfwGetWindowSize(m_Window, &windowWidth, &windowHeight);
            m_Data.Width = static_cast<uint32_t>(windowWidth);
            m_Data.Height = static_cast<uint32_t>(windowHeight);

            int framebufferWidth = 0;
            int framebufferHeight = 0;
            glfwGetFramebufferSize(m_Window, &framebufferWidth, &framebufferHeight);
            m_Data.FramebufferWidth = static_cast<uint32_t>(framebufferWidth);
            m_Data.FramebufferHeight = static_cast<uint32_t>(framebufferHeight);
        }

        glfwSetWindowUserPointer(m_Window, &m_Data);

        SetVSync(true);

        // 逻辑客户区尺寸；渲染/UI 视口以帧缓冲回调为准。
        glfwSetWindowSizeCallback(m_Window,
                                  [](GLFWwindow *window, int width, int height)
                                  {
                                      WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
                                      data.Width = static_cast<uint32_t>(width);
                                      data.Height = static_cast<uint32_t>(height);

                                      int framebufferWidth = 0;
                                      int framebufferHeight = 0;
                                      glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
                                      data.FramebufferWidth = static_cast<uint32_t>(framebufferWidth);
                                      data.FramebufferHeight = static_cast<uint32_t>(framebufferHeight);

                                      if (data.EventCallback)
                                      {
                                          WindowResizeEvent event(framebufferWidth, framebufferHeight);
                                          data.EventCallback(event);
                                      }
                                  });

        glfwSetFramebufferSizeCallback(m_Window,
                                       [](GLFWwindow *window, int width, int height)
                                       {
                                           WindowData &data =
                                                   *(WindowData *)glfwGetWindowUserPointer(window);
                                           data.FramebufferWidth = static_cast<uint32_t>(width);
                                           data.FramebufferHeight = static_cast<uint32_t>(height);

                                           if (data.EventCallback)
                                           {
                                               WindowResizeEvent event(width, height);
                                               data.EventCallback(event);
                                           }
                                       });

        glfwSetWindowCloseCallback(m_Window,
                                   [](GLFWwindow *window)
                                   {
                                       WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
                                       WindowCloseEvent event;
                                       data.EventCallback(event);
                                   });

        glfwSetKeyCallback(m_Window,
                           [](GLFWwindow *window, int key, int scancode, int action, int mods)
                           {
                               WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
                               switch (action)
                               {
                                   case GLFW_PRESS:
                                   {
                                       KeyPressedEvent event(key, 0);
                                       data.EventCallback(event);
                                       break;
                                   }
                                   case GLFW_RELEASE:
                                   {
                                       KeyReleasedEvent event(key);
                                       data.EventCallback(event);
                                       break;
                                   }
                                   case GLFW_REPEAT:
                                   {
                                       KeyPressedEvent event(key, 1);
                                       data.EventCallback(event);
                                       break;
                                   }
                               }
                           });
        glfwSetCharCallback(m_Window,
                            [](GLFWwindow *window, unsigned int keycode)
                            {
                                WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

                                KeyTypedEvent event(keycode);
                                data.EventCallback(event);
                            });

        glfwSetMouseButtonCallback(m_Window,
                                   [](GLFWwindow *window, int button, int action, int mods)
                                   {
                                       WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

                                       switch (action)
                                       {
                                           case GLFW_PRESS:
                                           {
                                               MouseButtonPressedEvent event(button);
                                               data.EventCallback(event);
                                               break;
                                           }
                                           case GLFW_RELEASE:
                                           {
                                               MouseButtonReleasedEvent event(button);
                                               data.EventCallback(event);
                                               break;
                                           }
                                       }
                                   });

        glfwSetScrollCallback(m_Window,
                              [](GLFWwindow *window, double xOffset, double yOffset)
                              {
                                  WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

                                  MouseScrolledEvent event((float)xOffset, (float)yOffset);
                                  data.EventCallback(event);
                              });

        glfwSetCursorPosCallback(m_Window,
                                 [](GLFWwindow *window, double xPos, double yPos)
                                 {
                                     WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

                                     MouseMovedEvent event((float)xPos, (float)yPos);
                                     if (data.EventCallback)
                                     {
                                         data.EventCallback(event);
                                     }
                                     else
                                     {
                                         HIMII_CORE_ERROR("Event callback is not set!");
                                     }
                                 });
    }

    void WindowsWindow::Shutdown()
    {
        HIMII_PROFILE_FUNCTION();

        glfwDestroyWindow(m_Window);
        --s_GLFWWindwCount;

        if (s_GLFWWindwCount == 0)
        {
            glfwTerminate();
        }
    }

    void WindowsWindow::Update()
    {
        HIMII_PROFILE_FUNCTION();

        m_Context->SwapBuffers();
        glfwPollEvents();
    }

    void WindowsWindow::MapWindowCursorToFramebufferPixels(float windowCursorX, float windowCursorY,
                                                           float &outFramebufferX,
                                                           float &outFramebufferY) const
    {
        if (m_Data.Width == 0 || m_Data.Height == 0)
        {
            outFramebufferX = windowCursorX;
            outFramebufferY = windowCursorY;
            return;
        }

        const float scaleX =
                static_cast<float>(m_Data.FramebufferWidth) / static_cast<float>(m_Data.Width);
        const float scaleY =
                static_cast<float>(m_Data.FramebufferHeight) / static_cast<float>(m_Data.Height);
        outFramebufferX = windowCursorX * scaleX;
        outFramebufferY = windowCursorY * scaleY;
    }

    void WindowsWindow::GetContentScale(float &scaleX, float &scaleY) const
    {
        scaleX = 1.0f;
        scaleY = 1.0f;
        if (m_Window)
            glfwGetWindowContentScale(m_Window, &scaleX, &scaleY);
    }

    void WindowsWindow::SetVSync(bool enabled)
    {
        HIMII_PROFILE_FUNCTION();

        if (enabled)
        {
            glfwSwapInterval(1); // 开启VSync
        }
        else
        {
            glfwSwapInterval(0); // 关闭VSync
        }
        m_Data.VSync = enabled;
    }

    bool WindowsWindow::IsVSync() const
    {
        return m_Data.VSync;
    }

    void WindowsWindow::SetTitle(const std::string &title)
    {
        m_Data.Title = title;
        if (m_Window)
            glfwSetWindowTitle(m_Window, title.c_str());
    }

    void WindowsWindow::SetClientSize(uint32_t width, uint32_t height)
    {
        m_Data.Width = width;
        m_Data.Height = height;
        if (m_Window)
            glfwSetWindowSize(m_Window, (int)width, (int)height);
    }

    void WindowsWindow::CenterOnScreen()
    {
        if (!m_Window)
            return;

        GLFWmonitor *primary_monitor = glfwGetPrimaryMonitor();
        if (!primary_monitor)
            return;

        const GLFWvidmode *video_mode = glfwGetVideoMode(primary_monitor);
        if (!video_mode)
            return;

        int position_x = (video_mode->width - (int)m_Data.Width) / 2;
        int position_y = (video_mode->height - (int)m_Data.Height) / 2;
        glfwSetWindowPos(m_Window, position_x, position_y);
    }

    void WindowsWindow::SetDecorated(bool decorated)
    {
        if (!m_Window)
            return;

        glfwSetWindowAttrib(m_Window, GLFW_DECORATED, decorated ? GLFW_TRUE : GLFW_FALSE);

#ifdef HIMII_PLATFORM_WINDOWS
        if (decorated)
        {
            HWND window_handle = glfwGetWin32Window(m_Window);
            ApplyDarkNativeTitleBar(window_handle);
        }
#endif
    }

    void WindowsWindow::MaximizeForEditor()
    {
        if (!m_Window)
            return;

        SetDecorated(true);
        glfwMaximizeWindow(m_Window);

        GLFWmonitor *primary_monitor = glfwGetPrimaryMonitor();
        if (primary_monitor)
        {
            const GLFWvidmode *video_mode = glfwGetVideoMode(primary_monitor);
            if (video_mode)
            {
                m_Data.Width = (uint32_t)video_mode->width;
                m_Data.Height = (uint32_t)video_mode->height;
            }
        }
    }

} // namespace Himii
