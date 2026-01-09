#include "Hepch.h"
#include "ContentBrowserPanel.h"
#include "Himii/Project/Project.h"

#include <imgui.h>

namespace Himii
{
    //extern const std::filesystem::path s_AssetsPath = "assets";

    ContentBrowserPanel::ContentBrowserPanel() : m_CurrentDirectory("")
    {
        m_DirectoryIcon = Texture2D::Create("resources/icons/Folder.png");
        m_FileIcon = Texture2D::Create("resources/icons/doc.png");
        m_ScriptIcon = Texture2D::Create("resources/icons/Script.png");
        m_SceneIcon = Texture2D::Create("resources/icons/Scene.png");
    }

    void ContentBrowserPanel::OnImGuiRender()
    {
        ImGui::Begin("Content Browser");

        if (!Project::GetActive())
        {
            ImGui::Text("Please open a project.");
            ImGui::End();
            return;
        }

        const std::filesystem::path& assetsPath = Project::GetAssetDirectory();

        // Ensure m_CurrentDirectory is valid
        if (m_CurrentDirectory.empty() || !std::filesystem::exists(m_CurrentDirectory))
            m_CurrentDirectory = assetsPath;

        // Split View Table
        if (ImGui::BeginTable("ContentBrowserTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
        {
            // --- Left Panel: Tree View ---
            ImGui::TableSetupColumn("Tree", ImGuiTableColumnFlags_WidthFixed, 200.0f);
            ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);
            
            ImGui::TableNextColumn();
            
            // Gradient Background for Tree (Shadow style)
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            float colWidth = ImGui::GetContentRegionAvail().x;
            // Shadow from separation line (right side) to left 10px
            ImVec2 gradientStart = ImVec2(p0.x + colWidth - 10.0f, p0.y);
            ImVec2 gradientEnd = ImVec2(p0.x + colWidth, p0.y + ImGui::GetContentRegionAvail().y + 5000.0f); // extend down
            
             ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
                gradientStart, gradientEnd, 
                ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f)), // Top Left (Transparent)
                ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.2f)), // Top Right (Shadow)
                ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.2f)), // Bot Right (Shadow)
                ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f))  // Bot Left (Transparent)
            );

            // Draw Tree
            DrawTree(assetsPath);

            // --- Right Panel: Grid View ---
            ImGui::TableNextColumn();
            
            // Navigation Bar (Breadcrumbs)
            std::vector<std::filesystem::path> breadcrumbs;
            std::filesystem::path currentNavPath = m_CurrentDirectory;
            while (currentNavPath != assetsPath.parent_path() && !currentNavPath.empty()) {
                breadcrumbs.push_back(currentNavPath);
                currentNavPath = currentNavPath.parent_path();
            }
            // Add assetsPath if loop stopped before it (it should stop at parent of assetsPath)
            // But if m_CurrentDirectory is assetsPath, loop adds it.
            // Wait, loop condition `currentNavPath != assetsPath.parent_path()` means it includes assetsPath.
            
            std::reverse(breadcrumbs.begin(), breadcrumbs.end());

            for (size_t i = 0; i < breadcrumbs.size(); ++i)
            {
                if (i > 0)
                {
                    ImGui::Text(">");
                    ImGui::SameLine();
                }

                std::string name = breadcrumbs[i].filename().string();
                if (breadcrumbs[i] == assetsPath) name = "Assets"; // Rename root
                
                if (ImGui::Button(name.c_str()))
                {
                    m_CurrentDirectory = breadcrumbs[i];
                }
                ImGui::SameLine();
            }
            ImGui::NewLine();
            ImGui::Separator();
            

            // Adaptive Flow Layout
            static float thumbnailSize = 64.0f;
            static float padding = 16.0f;
            float cellSize = thumbnailSize + padding;
            
            float panelWidth = ImGui::GetContentRegionAvail().x;
            int columnCount = (int)(panelWidth / cellSize);
            if (columnCount < 1) columnCount = 1;

            int column = 0;
            for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
            {
                const auto& path = directoryEntry.path();
                auto relativePath = std::filesystem::relative(path, assetsPath);
                std::string fileNameString = relativePath.filename().string();
                
                Ref<Texture2D> icon = m_FileIcon;
                if (directoryEntry.is_directory())
                    icon = m_DirectoryIcon;
                else if (path.extension() == ".cs")
                    icon = m_ScriptIcon;
                else if (path.extension() == ".himii")
                    icon = m_SceneIcon;

                ImGui::PushID(fileNameString.c_str());
                
                // Group the icon and text together for the flow layout
                ImGui::BeginGroup();
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::ImageButton("BtnId", (ImTextureID)icon->GetRendererID(), {thumbnailSize, thumbnailSize}, {0, 1}, {1, 0});
                ImGui::PopStyleColor();

                if (ImGui::BeginDragDropSource())
                {
                    const char* itemPath = relativePath.string().c_str();
                    ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (std::strlen(itemPath) + 1));
                    ImGui::EndDragDropSource();
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (directoryEntry.is_directory())
                        m_CurrentDirectory /= path.filename();
                }
                
                // Center Text
                float textWidth = ImGui::CalcTextSize(fileNameString.c_str()).x;
                if (textWidth < thumbnailSize)
                {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (thumbnailSize - textWidth) * 0.5f);
                }
                ImGui::TextWrapped(fileNameString.c_str());
                
                ImGui::EndGroup();
                
                ImGui::PopID();
                
                column++;
                if (column < columnCount)
                {
                    ImGui::SameLine();
                }
                else
                {
                    column = 0;
                }
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }

    void ContentBrowserPanel::DrawTree(const std::filesystem::path& path)
    {
        // Name Computation
        std::string filename = path.filename().string();
        if (filename.empty()) filename = path.string(); // Fallback for root if needed, though assetsPath usually has name
        
        // Tree Node Flags
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
        
        // Check if this path is part of the current path to auto-open or highlight?
        // For now just basic tree.
        if (m_CurrentDirectory == path)
            flags |= ImGuiTreeNodeFlags_Selected;
        
        // If directory is empty, maybe make it a leaf?
        // But iterating to check emptiness might be slow. Let's just assume folders are folders.
        
        bool opened = ImGui::TreeNodeEx(filename.c_str(), flags);
        
        if (ImGui::IsItemClicked())
        {
            m_CurrentDirectory = path;
        }

        if (opened)
        {
            for (auto& directoryEntry : std::filesystem::directory_iterator(path))
            {
                if (directoryEntry.is_directory())
                {
                    DrawTree(directoryEntry.path());
                }
            }
            ImGui::TreePop();
        }
    }
    void ContentBrowserPanel::Refresh()
    {
        if (Project::GetActive())
        {
            m_CurrentDirectory = Project::GetAssetDirectory();
        }
    }
} // namespace Himii
