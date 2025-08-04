#include "Hepch.h"
#include "Himii/Core/Log.h"
#include "WindowsWindow.h"
#include "Himii/Core/Input.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include "Himii/Events/ApplicationEvent.h"
#include "Himii/Events/KeyEvent.h"
#include "Himii/Events/MouseEvent.h"

namespace Himii
{
    static uint8_t s_GLFWWindwCount = 0;

    static void GLFWErrorCallback(int error, const char *description)
    {
        HIMII_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
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
        // 设置标题
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        HIMII_CORE_INFO("Creating window {0} ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);

        if (s_GLFWWindwCount==0)
        {
            HIMII_CORE_INFO("glfwInit");
            int sucess = glfwInit();
            HIMII_CORE_ASSERT(sucess, "Failed to initialize GLFW");
            glfwSetErrorCallback(GLFWErrorCallback);
        }

        m_Window = glfwCreateWindow((int)props.Width, props.Height, m_Data.Title.c_str(), nullptr, nullptr);
        ++s_GLFWWindwCount;

        m_Context = CreateScope<OpenGLContext>(m_Window);
        m_Context->Init();

        if (!m_Window)
        {
            HIMII_CORE_ERROR("Failed to create GLFW window");
            glfwTerminate();
            return;
        }
        glfwSetWindowUserPointer(m_Window, &m_Data);

        SetVSync(true);

        // 设置窗口回调
        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow *window, int width, int height)
        {
            WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
            data.Width = width;
            data.Height = height;
            WindowResizeEvent event(width, height);
            data.EventCallback(event);
        });

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow *window)
        {
            WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
            WindowCloseEvent event;
            data.EventCallback(event);
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
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
        glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(keycode);
			data.EventCallback(event);
		});

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

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

        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			if (data.EventCallback) {
                data.EventCallback(event);
            } 
            else {
                HIMII_CORE_ERROR("Event callback is not set!");
            }

		});
    }

    void WindowsWindow::Shutdown()
    {
        glfwDestroyWindow(m_Window);
        --s_GLFWWindwCount;

        if (s_GLFWWindwCount == 0)
        {
            glfwTerminate();
        }
    }

    void WindowsWindow::Update()
    {
        m_Context->SwapBuffers();
        glfwPollEvents();
    }
    void WindowsWindow::SetVSync(bool enabled)
    {
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

} // namespace Himii
