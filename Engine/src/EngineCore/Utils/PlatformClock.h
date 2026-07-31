#pragma once

namespace Himii
{
    /// 与窗口后端无关的单调时钟（主循环用）。
    class PlatformClock
    {
    public:
        static double GetTimeSeconds();
    };
}
