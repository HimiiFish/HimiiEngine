#include "Window.h"
#include "Hepch.h"
#include "Platform/Windows/WindowsWindow.h"

namespace Himii
{
    Scope<Window> Window::Create(const WindowProps &props)
    {
        return CreateScope<WindowsWindow>(props);
    }
} // namespace Core

