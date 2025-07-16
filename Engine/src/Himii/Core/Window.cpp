#include "Window.h"
#include "Hepch.h"
#include "Platform/Windows/WindowsWindow.h"

namespace Himii
{
    std::unique_ptr<Window> Window::Create(const WindowProps &props)
    {
        return CreateScope<WindowsWindow>(props);
    }
} // namespace Core

