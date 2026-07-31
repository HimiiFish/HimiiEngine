#include "Hepch.h"
#include "EngineCore/Utils/PlatformClock.h"

#include <chrono>

namespace Himii
{
    double PlatformClock::GetTimeSeconds()
    {
        static const auto startTime = std::chrono::steady_clock::now();
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - startTime).count();
    }
}
