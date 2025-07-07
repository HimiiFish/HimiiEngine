#include "Window.h"
#include "Hepch.h"

namespace Himii
{
    std::unique_ptr<Window> Window::Create(const WindowProps &props)
    {
#ifdef PLATFORM_WINDOWS
        return CreateScope<Window>(props);
#else
        LOG_CORE_ERROR("Unknown platform!")
#endif // PLATFORM_WINDOWS
    }
} // namespace Core
