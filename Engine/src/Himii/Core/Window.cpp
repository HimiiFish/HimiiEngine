#include "Window.h"
#include "Hepch.h"
#include "Platform/WindowsWindow.h"

namespace Himii
{
    std::unique_ptr<Window> Window::Create(const WindowProps &props)
    {
#ifdef PLATFORM_WINDOWS
        return CreateScope<WindowsWindow>(props);
#else
        LOG_CORE_ERROR("Unknown platform!");
#endif // PLATFORM_WINDOWS
    }
} // namespace Core

