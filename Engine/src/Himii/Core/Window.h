#pragma once

#include "Himii/Events/Event.h"
#include <sstream>
#include "Log.h"


namespace Engine
{
    struct WindowProps 
    {
        const char* Title;
        uint32_t Width;
        uint32_t Height;

        WindowProps(const char* title = "Himii Engine", uint32_t width = 1600, uint32_t height = 900) :
            Title(title), Width(width), Height(height)
        {
        }
    };
    /// <summary>
    /// 所有窗口类的基类，提供基础的接口API方法
    /// </summary>
    class Window {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        virtual ~Window()=default;

        virtual void OnUpdate() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void SetEventCallback(const EventCallbackFn &callback) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        virtual void *GetNativeWindow() const = 0;

        static Scope<Window> Create(const WindowProps &props = WindowProps());

    };
}