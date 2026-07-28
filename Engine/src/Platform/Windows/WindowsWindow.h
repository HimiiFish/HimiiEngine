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

        uint32_t GetFramebufferWidth() const override
        {
            return m_Data.FramebufferWidth;
        };

        uint32_t GetFramebufferHeight() const override
        {
            return m_Data.FramebufferHeight;
        };

        void MapWindowCursorToFramebufferPixels(float windowCursorX, float windowCursorY,
                                                float &outFramebufferX,
                                                float &outFramebufferY) const override;

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

        void SetTitle(const std::string &title) override;

        void SetClientSize(uint32_t width, uint32_t height) override;

        void CenterOnScreen() override;

        void MaximizeForEditor() override;

        void SetDecorated(bool decorated) override;

    private:
        virtual void Init(const WindowProps &props);
        virtual void Shutdown();

    private:
        GLFWwindow *m_Window;
        Scope<GraphicsContext> m_Context;

        struct WindowData {
            std::string Title;
            uint32_t Width, Height;
            uint32_t FramebufferWidth = 0;
            uint32_t FramebufferHeight = 0;
            bool VSync;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };

} // namespace Himii
