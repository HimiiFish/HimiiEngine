#pragma once
#include "Himii/Core/Window.h"

#include <SDL3/SDL.h>

namespace Himii {

class WindowsWindow : public Window {
public:
    WindowsWindow(const WindowProps& props);
    virtual ~WindowsWindow();

    void Update() override;
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;
    void SetEventCallback(const EventCallbackFn &callback) override;
    void SetVSync(bool enabled) override;
    bool IsVSync() const override;
    void * GetNativeWindow() const override;

private:
    virtual void Init(const WindowProps& props);
    virtual void Shutdown();

    SDL_Window* m_Window;
    SDL_GLContext m_Context;

    struct WindowData {
        std::string Title;
        uint32_t Width, Height;
        bool VSync;
        EventCallbackFn EventCallback;
    };

    WindowData m_Data;
};

}
