#pragma once
#include "Himii/Core/Window.h"
#include "SDL3/SDL.h"

namespace Engine
{
    class WindowsWindow : public Window {
    public:
        WindowsWindow(const WindowProps &props);
        virtual ~WindowsWindow();

        void OnUpdate() override;

        unsigned int GetWidth() const override
        {
            return m_Data.Width;
        }
        unsigned int GetHeight() const override
        {
            return m_Data.Height;
        }

        void SetEventCallback(const EventCallbackFn &callback) override
        {
            m_Data.EventCallback = callback;
        }
        void SetVSync(bool enabled) override;
        bool IsVSync() const override;

        virtual void *GetNativeWindow() const
        {
            return m_Window;
        }
    private:
        virtual void Init(const WindowProps &props);
        virtual void Shutdown();
    private:
        SDL_Window *m_Window;

        struct WindowData 
        {
            const char* Title;
            unsigned int Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };
        WindowData m_Data;
    };
} // namespace Himii
