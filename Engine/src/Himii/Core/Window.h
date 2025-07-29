#pragma once

#include "Himii/Events/Event.h"
#include "Himii/Events/ApplicationEvent.h"
#include "Himii/Events/KeyEvent.h"
#include "Himii/Events/MouseEvent.h"
#include <sstream>
#include "Log.h"
#include "Himii/Core/Core.h"
#include "Himii/Events/Event.h"

namespace Himii
{
    struct WindowProps 
    {
        std::string Title;
        uint32_t Width;
        uint32_t Height;

        WindowProps(const std::string &title = "Himii Engine", uint32_t width = 1280, uint32_t height = 720) :
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

        virtual void Update() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void SetEventCallback(const EventCallbackFn &callback) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        virtual void *GetNativeWindow() const = 0;

        static Scope<Window> Create(const WindowProps &props = WindowProps());

    };
}