#pragma once

#include "EngineCore/Events/Event.h"
#include "EngineCore/Events/ApplicationEvent.h"
#include "EngineCore/Events/KeyEvent.h"
#include "EngineCore/Events/MouseEvent.h"
#include <sstream>

namespace Himii
{
    struct WindowProps
    {
        std::string Title;
        uint32_t Width;
        uint32_t Height;
        bool Decorated = true;
        bool Maximized = true;
        bool CenterOnScreen = false;

        WindowProps(const std::string &title = "Himii Engine", uint32_t width = 1280, uint32_t height = 720) :
            Title(title), Width(width), Height(height)
        {
        }
    };

    class Window
    {
    public:
        using EventCallbackFn = std::function<void(Event &)>;

        virtual ~Window() = default;

        virtual void Update() = 0;

        /// Client area size in window/logical pixels.
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        /// Default framebuffer size in physical pixels.
        virtual uint32_t GetFramebufferWidth() const = 0;
        virtual uint32_t GetFramebufferHeight() const = 0;

        /// Maps cursor position (window/logical pixels, Y-down) to framebuffer pixels (Y-down).
        virtual void MapWindowCursorToFramebufferPixels(float windowCursorX, float windowCursorY,
                                                        float &outFramebufferX,
                                                        float &outFramebufferY) const = 0;

        /// DPI / 高分屏内容缩放（逻辑像素 → 物理像素倍率）。
        virtual void GetContentScale(float &scaleX, float &scaleY) const = 0;

        virtual void SetEventCallback(const EventCallbackFn &callback) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        virtual void *GetNativeWindow() const = 0;

        virtual void SetTitle(const std::string &title) {}

        virtual void SetClientSize(uint32_t width, uint32_t height) {}

        virtual void CenterOnScreen() {}

        virtual void MaximizeForEditor() {}

        virtual void SetDecorated(bool decorated) {}

        static Scope<Window> Create(const WindowProps &props = WindowProps());
    };
}
