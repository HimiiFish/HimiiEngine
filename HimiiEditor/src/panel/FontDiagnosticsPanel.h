#pragma once

#include "Module/Render/Renderer/Font.h"
#include <string>

namespace Himii
{
    class FontDiagnosticsPanel
    {
    public:
        void OnImGuiRender();
        void SetFont(const Ref<Font> &font)
        {
            m_Font = font;
        }
        bool &GetShowFlag()
        {
            return m_Show;
        }

    private:
        Ref<Font> m_Font;
        bool m_Show = false;
        int m_SelectedPageIndex = 0;
    };
}
