#pragma once
#include <chrono>

namespace Himii
{
    class Timer {
    public:
        Timer()
        {
            Reset();
        }

        void Timer::Reset()
        {
            m_start = std::chrono::high_resolution_clock::now();
        }

        float Timer::Elapsed() const
        {
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - m_start;
            return duration.count();
        }

        float Timer::ElapsedMillis() const
        {
            return Elapsed() * 1000.0f;
        }

    private:
        std::chrono::high_resolution_clock::time_point m_start;
    };
}
