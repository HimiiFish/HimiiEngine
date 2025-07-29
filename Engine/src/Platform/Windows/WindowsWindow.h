#pragma once
#include "GLFW/glfw3.h"
#include "Himii/Core/Window.h"
#include "Himii/Renderer/GraphicsContext.h"

namespace Himii
{
    class WindowsWindow : public Window {
    public:
        WindowsWindow(const WindowProps &props);
        virtual ~WindowsWindow();

        void Update() override;

        uint32_t GetWidth() const override
        {
            return m_Data.Width;
        };
        uint32_t GetHeight() const override
        {
            return m_Data.Height;
        };

        void SetEventCallback(const EventCallbackFn &callback) override
        {
            m_Data.EventCallback = callback;
        };
        void SetVSync(bool enabled) override;
        bool IsVSync() const override;

        virtual void *GetNativeWindow() const override
        {
            return m_Window;
        };
    private:
        virtual void Init(const WindowProps &props);
        virtual void Shutdown();

    private:
        GLFWwindow *m_Window;
        Scope<GraphicsContext> m_Context;

        struct WindowData {
            std::string Title;
            uint32_t Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };

} // namespace Himii
