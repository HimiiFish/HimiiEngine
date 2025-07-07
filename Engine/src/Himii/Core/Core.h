#pragma once
#define BIT(x) (1 << x)

#define PLATFORM_WINDOWS

namespace Himii
{
    template<typename T>
    using Scope = std::unique_ptr<T>;
    template<typename T, typename... Args>
    constexpr Scope<T> CreateScope(Args &&...args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
}
