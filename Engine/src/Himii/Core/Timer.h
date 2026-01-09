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

        void Reset()
        {
            m_start = std::chrono::high_resolution_clock::now();
        }

        float Elapsed() const
        {
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - m_start;
            return duration.count();
        }

        float ElapsedMillis() const
        {
            return Elapsed() * 1000.0f;
        }

    private:
        std::chrono::high_resolution_clock::time_point m_start;
    };
}
