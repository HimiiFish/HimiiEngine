#pragma once

namespace Himii
{
    class ConsolePanel
    {
    public:
        void OnImGuiRender(bool *open);

    private:
        bool m_ShowScript = true;
        bool m_ShowCompile = true;
        bool m_ShowEngine = true;
        bool m_ShowEngineVerbose = false;
        bool m_AutoScroll = true;
    };
}
