#pragma once
#include "Himii/Renderer/Texture.h"

#include "filesystem"

namespace Himii
{
    class ContentBrowserPanel {
    public:
        ContentBrowserPanel();

        void OnImGuiRender();

        void Refresh();
    private:
        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;

        Ref<Texture2D> m_DirectoryIcon;
        Ref<Texture2D> m_FileIcon;
    };
}
