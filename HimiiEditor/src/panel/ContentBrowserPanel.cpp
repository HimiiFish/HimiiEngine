#include "Hepch.h"
#include "ContentBrowserPanel.h"

#include <imgui.h>

namespace Himii
{
    extern const std::filesystem::path s_AssetsPath = "assets";

    ContentBrowserPanel::ContentBrowserPanel() : m_CurrentDirectory(s_AssetsPath)
    {
        m_DirectoryIcon = Texture2D::Create("resources/icons/Folder.png");
        m_FileIcon = Texture2D::Create("resources/icons/doc.png");
    }

    void ContentBrowserPanel::OnImGuiRender()
    {
        ImGui::Begin("Content Browser");
        
        if (m_CurrentDirectory != std::filesystem::path(s_AssetsPath))
        {
            if (ImGui::Button("<"))
            {
                m_CurrentDirectory = m_CurrentDirectory.parent_path();
            }
        }

        static float padding = 16.0f;
        static float thumbnailSize = 128.0f;
        float cellSize = thumbnailSize + padding;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1)
            columnCount = 1;

        ImGui::Columns(columnCount, 0, false);
        
        for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
        {
            const auto &path = directoryEntry.path();
            auto relativePath= std::filesystem::relative(path, s_AssetsPath);
            std::string fileNameString = relativePath.filename().string();
            
            Ref<Texture2D> icon = directoryEntry.is_directory() ? m_DirectoryIcon : m_FileIcon;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushID(fileNameString.c_str()); 
            ImGui::ImageButton("BtnId",(ImTextureID)icon->GetRendererID(), {thumbnailSize, thumbnailSize}, {0, 1}, {1, 0});

            if (ImGui::BeginDragDropSource())
            {
                const wchar_t *itemPath = relativePath.c_str();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM",itemPath,(wcslen(itemPath)+1)*sizeof(wchar_t));
                 
                ImGui::EndDragDropSource();
            }

            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (directoryEntry.is_directory())
                {
                    m_CurrentDirectory /= path.filename();
                }
                else
                {
                    // Open the file with the default application
					#ifdef _WIN32
						std::system(("start \"\" \"" + path.string() + "\"").c_str());
					#elif __APPLE__
						std::system(("open \"" + path.string() + "\"").c_str());
					#else
						std::system(("xdg-open \"" + path.string() + "\"").c_str());
					#endif
                }
            }
            ImGui::TextWrapped(fileNameString.c_str());
            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);

        ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
        ImGui::SliderFloat("Padding", &padding, 0, 32);

        ImGui::End();
    }
} // namespace Himii
