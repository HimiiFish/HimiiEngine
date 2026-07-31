#include "Hepch.h"
#include "ContentBrowserPanel.h"
#include "EditorExternalFileDrop.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "Resource/AssetManager.h"
#include "Resource/SpriteSheetUtility.h"
#include "EngineCore/Utils/PlatformUtils.h"
#include "EngineCore/Core/Log.h"

#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <imgui.h>

namespace Himii
{

    static bool IsImageFileExtension(const std::filesystem::path& filePath)
    {
        std::string extension = filePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp"
               || extension == ".tga";
    }

    bool ContentBrowserPanel::ShouldHideFromContentBrowser(const std::filesystem::path& path)
    {
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return extension == ".meta";
    }

    Ref<Texture2D> ContentBrowserPanel::GetOrLoadImageThumbnail(
            const std::filesystem::path& relativePath)
    {
        const std::string cacheKey = relativePath.generic_string();
        const auto cacheIterator = m_ImageThumbnailCache.find(cacheKey);
        if (cacheIterator != m_ImageThumbnailCache.end())
            return cacheIterator->second;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return nullptr;

        const AssetHandle textureHandle = assetManager->ImportAsset(relativePath);
        if (textureHandle == 0)
            return nullptr;

        Ref<Asset> asset = assetManager->GetAsset(textureHandle);
        if (!asset)
            return nullptr;

        Ref<Texture2D> texture = std::static_pointer_cast<Texture2D>(asset);
        m_ImageThumbnailCache.emplace(cacheKey, texture);
        return texture;
    }

    static void DrawAspectFitThumbnail(const Ref<Texture2D>& texture, const ImVec2& boxMin, float boxSize)
    {
        if (!texture)
            return;

        const uint32_t textureWidth = texture->GetWidth();
        const uint32_t textureHeight = texture->GetHeight();
        if (textureWidth == 0 || textureHeight == 0)
            return;

        const float sourceWidth = static_cast<float>(textureWidth);
        const float sourceHeight = static_cast<float>(textureHeight);
        const float scale = std::min(boxSize / sourceWidth, boxSize / sourceHeight);
        const float displayWidth = sourceWidth * scale;
        const float displayHeight = sourceHeight * scale;

        const ImVec2 imageMin(boxMin.x + (boxSize - displayWidth) * 0.5f,
                              boxMin.y + (boxSize - displayHeight) * 0.5f);
        const ImVec2 imageMax(imageMin.x + displayWidth, imageMin.y + displayHeight);

        const auto& textureUvCorners = SpriteSheetUtility::FullTextureImGuiUvCorners;
        ImGui::GetWindowDrawList()->AddImage(
                (ImTextureID)(intptr_t)texture->GetRendererID(), imageMin, imageMax,
                ImVec2(textureUvCorners.TopLeft.x, textureUvCorners.TopLeft.y),
                ImVec2(textureUvCorners.BottomRight.x, textureUvCorners.BottomRight.y));
    }

    static std::filesystem::path NormalizePath(const std::filesystem::path& path)
    {
        std::error_code errorCode;
        std::filesystem::path normalizedPath = std::filesystem::weakly_canonical(path, errorCode);
        if (errorCode)
            normalizedPath = path.lexically_normal();
        return normalizedPath;
    }

    std::string ContentBrowserPanel::TruncateTextToWidth(const char* text, float maxWidth)
    {
        if (!text || text[0] == '\0')
            return {};

        const ImVec2 fullTextSize = ImGui::CalcTextSize(text);
        if (fullTextSize.x <= maxWidth)
            return text;

        constexpr const char* ellipsis = "...";
        const float ellipsisWidth = ImGui::CalcTextSize(ellipsis).x;
        const float maximumTextWidth = maxWidth - ellipsisWidth;
        if (maximumTextWidth <= 0.0f)
            return ellipsis;

        const size_t textLength = std::strlen(text);
        size_t lowIndex = 0;
        size_t highIndex = textLength;
        size_t bestFitIndex = 0;

        while (lowIndex <= highIndex)
        {
            const size_t middleIndex = lowIndex + (highIndex - lowIndex) / 2;
            const ImVec2 partialTextSize = ImGui::CalcTextSize(text, text + middleIndex);
            if (partialTextSize.x <= maximumTextWidth)
            {
                bestFitIndex = middleIndex;
                lowIndex = middleIndex + 1;
            }
            else
            {
                if (middleIndex == 0)
                    break;
                highIndex = middleIndex - 1;
            }
        }

        std::string truncatedText(text, bestFitIndex);
        truncatedText += ellipsis;
        return truncatedText;
    }

    bool ContentBrowserPanel::IsOnPathToCurrentDirectory(const std::filesystem::path& path) const
    {
        const std::filesystem::path normalizedPath = NormalizePath(path);
        const std::filesystem::path normalizedCurrentDirectory = NormalizePath(m_CurrentDirectory);

        if (normalizedPath == normalizedCurrentDirectory)
            return true;

        std::error_code errorCode;
        const std::filesystem::path relativePath =
                std::filesystem::relative(normalizedCurrentDirectory, normalizedPath, errorCode);
        if (errorCode || relativePath.empty())
            return false;

        const std::string relativeString = relativePath.generic_string();
        return relativeString.rfind("..", 0) != 0;
    }

    void ContentBrowserPanel::DrawContentDetailBar(float barWidth)
    {
        const ImGuiStyle& style = ImGui::GetStyle();
        const float detailBarHeight = ImGui::GetTextLineHeight() + style.WindowPadding.y * 2.0f;
        const ImVec2 detailBarMin = ImGui::GetCursorScreenPos();
        const ImVec2 detailBarMax(detailBarMin.x + barWidth, detailBarMin.y + detailBarHeight);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(
                detailBarMin, detailBarMax,
                ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.38f)));
        drawList->AddLine(
                ImVec2(detailBarMin.x, detailBarMin.y),
                ImVec2(detailBarMax.x, detailBarMin.y),
                ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.08f)));

        ImGui::Dummy(ImVec2(barWidth, detailBarHeight));

        if (!m_SelectedItemDisplayName.empty())
        {
            ImGui::SetCursorScreenPos(
                    ImVec2(detailBarMin.x + style.WindowPadding.x,
                           detailBarMin.y + style.WindowPadding.y));
            ImGui::TextUnformatted(m_SelectedItemDisplayName.c_str());
        }
    }

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

            if (ImGui::BeginChild("##ContentBrowserTree", ImVec2(0.0f, 0.0f), false))
            {
                DrawTree(assetsPath, assetsPath);
            }
            ImGui::EndChild();

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

            static bool openCreateScriptPopup = false;
            static char scriptName[128] = "NewScript";

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    (void)payload;
                }
                ImGui::EndDragDropTarget();
            }

            std::vector<std::filesystem::path> droppedPaths = EditorExternalFileDrop::ConsumePendingPaths();
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && !droppedPaths.empty())
                ImportFilesFromPaths(droppedPaths);

            if (ImGui::BeginPopupContextWindow("ContentBrowserContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::MenuItem("Create C# Script"))
                    openCreateScriptPopup = true;
                if (ImGui::MenuItem("Import Asset..."))
                {
                    std::string selectedPath = FileDialog::OpenFile(
                        "Images (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0"
                        "Animations (*.anim)\0*.anim\0"
                        "Tile Sets (*.tileset)\0*.tileset\0"
                        "Tile Maps (*.tilemap)\0*.tilemap\0"
                        "Particle Emitters (*.particle)\0*.particle\0"
                        "C# Scripts (*.cs)\0*.cs\0"
                        "All Files (*.*)\0*.*\0");
                    if (!selectedPath.empty())
                        ImportFilesFromPaths({selectedPath});
                }
                ImGui::EndPopup();
            }

            if (openCreateScriptPopup)
                ImGui::OpenPopup("CreateScriptPopup");

            if (ImGui::BeginPopupModal("CreateScriptPopup", &openCreateScriptPopup, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::InputText("Class Name", scriptName, sizeof(scriptName));
                if (ImGui::Button("Create"))
                {
                    if (CreateCSharpScript(m_CurrentDirectory, scriptName))
                    {
                        if (m_OnScriptChanged)
                            m_OnScriptChanged();
                    }
                    openCreateScriptPopup = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    openCreateScriptPopup = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (NormalizePath(m_LastBrowsedDirectory) != NormalizePath(m_CurrentDirectory))
            {
                m_SelectedItemDisplayName.clear();
                m_LastBrowsedDirectory = m_CurrentDirectory;
            }

            static float thumbnailSize = 64.0f;
            static float padding = 16.0f;
            const float cellWidth = thumbnailSize + padding;
            const float detailBarHeight =
                    ImGui::GetTextLineHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
            const float contentColumnWidth = ImGui::GetContentRegionAvail().x;

            if (ImGui::BeginChild("##ContentBrowserGrid", ImVec2(0.0f, -detailBarHeight), false))
            {
                const float panelWidth = ImGui::GetContentRegionAvail().x;
                int columnCount = static_cast<int>((panelWidth + padding) / (cellWidth + padding));
                if (columnCount < 1)
                    columnCount = 1;

                int column = 0;
                for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
                {
                    const auto& path = directoryEntry.path();
                    if (!directoryEntry.is_directory() && ShouldHideFromContentBrowser(path))
                        continue;

                    auto relativePath = std::filesystem::relative(path, assetsPath);
                    std::string fileNameString = relativePath.filename().string();

                    Ref<Texture2D> icon = m_FileIcon;
                    if (directoryEntry.is_directory())
                        icon = m_DirectoryIcon;
                    else if (path.extension() == ".cs")
                        icon = m_ScriptIcon;
                    else if (path.extension() == ".himii")
                        icon = m_SceneIcon;
                    else if (path.extension() == ".hprefab")
                        icon = m_SceneIcon;

                    Ref<Texture2D> imageThumbnail;
                    if (!directoryEntry.is_directory() && IsImageFileExtension(path))
                        imageThumbnail = GetOrLoadImageThumbnail(relativePath);

                    ImGui::PushID(fileNameString.c_str());

                    if (column > 0)
                        ImGui::SameLine(0.0f, padding);

                    const bool isItemSelected = m_SelectedItemDisplayName == fileNameString;
                    const float cellContentHeight =
                            thumbnailSize + ImGui::GetStyle().ItemSpacing.y + ImGui::GetTextLineHeight();

                    if (isItemSelected)
                    {
                        const ImVec2 highlightMin = ImGui::GetCursorScreenPos();
                        const ImVec2 highlightMax(highlightMin.x + cellWidth,
                                                  highlightMin.y + cellContentHeight);
                        ImDrawList* drawList = ImGui::GetWindowDrawList();
                        drawList->AddRectFilled(
                                highlightMin, highlightMax,
                                ImGui::GetColorU32(ImGuiCol_Header, 0.55f), 4.0f);
                        drawList->AddRect(
                                highlightMin, highlightMax,
                                ImGui::GetColorU32(ImGuiCol_NavHighlight), 4.0f, 0, 1.5f);
                    }

                    ImGui::BeginGroup();

                    const float cellStartX = ImGui::GetCursorPosX();
                    const float thumbnailOffsetX = (cellWidth - thumbnailSize) * 0.5f;
                    ImGui::SetCursorPosX(cellStartX + thumbnailOffsetX);

                    ImGui::InvisibleButton("##Thumbnail", ImVec2(thumbnailSize, thumbnailSize));
                    const ImVec2 thumbnailBoxMin = ImGui::GetItemRectMin();
                    DrawAspectFitThumbnail(imageThumbnail ? imageThumbnail : icon, thumbnailBoxMin,
                                           thumbnailSize);

                    if (ImGui::BeginDragDropSource())
                    {
                        std::wstring itemPath = relativePath.wstring();
                        ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath.c_str(),
                                                  (itemPath.size() + 1) * sizeof(wchar_t));
                        ImGui::EndDragDropSource();
                    }

                    const bool thumbnailDoubleClicked =
                            ImGui::IsItemHovered()
                            && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                    const bool thumbnailClicked =
                            ImGui::IsItemClicked(ImGuiMouseButton_Left) && !thumbnailDoubleClicked;

                    if (thumbnailDoubleClicked)
                    {
                        if (directoryEntry.is_directory())
                        {
                            m_CurrentDirectory /= path.filename();
                        }
                        else if (IsImageFileExtension(path))
                        {
                            if (auto assetManager = ResourceSystem::GetAssetManager())
                            {
                                const AssetHandle textureHandle = assetManager->ImportAsset(relativePath);
                                if (textureHandle != 0)
                                    m_TextureInspectorRequest = textureHandle;
                            }
                        }
                        else if (path.extension() == ".anim")
                        {
                            m_AnimationEditorRequest = Project::GetAssetFileSystemPath(relativePath);
                        }
                    }
                    else if (thumbnailClicked)
                    {
                        m_SelectedItemDisplayName = fileNameString;
                    }

                    const std::string truncatedFileName = TruncateTextToWidth(fileNameString.c_str(), cellWidth);
                    const ImVec2 truncatedLabelSize = ImGui::CalcTextSize(truncatedFileName.c_str());
                    ImGui::SetCursorPosX(cellStartX + (cellWidth - truncatedLabelSize.x) * 0.5f);
                    ImGui::TextUnformatted(truncatedFileName.c_str());

                    const bool labelDoubleClicked =
                            ImGui::IsItemHovered()
                            && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                    const bool labelClicked =
                            ImGui::IsItemClicked(ImGuiMouseButton_Left) && !labelDoubleClicked;

                    if (labelDoubleClicked)
                    {
                        if (directoryEntry.is_directory())
                            m_CurrentDirectory /= path.filename();
                    }
                    else if (labelClicked)
                    {
                        m_SelectedItemDisplayName = fileNameString;
                    }

                    ImGui::Dummy(ImVec2(cellWidth, 0.0f));

                    ImGui::EndGroup();

                    ImGui::PopID();

                    column++;
                    if (column >= columnCount)
                        column = 0;
                }
            }
            ImGui::EndChild();

            DrawContentDetailBar(contentColumnWidth);

            ImGui::EndTable();
        }

        ImGui::End();
    }

    void ContentBrowserPanel::DrawTree(const std::filesystem::path& path,
                                       const std::filesystem::path& assetsPath)
    {
        std::string displayName = path.filename().string();
        if (displayName.empty())
            displayName = path.string();
        if (NormalizePath(path) == NormalizePath(assetsPath))
            displayName = "Assets";

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
                                   | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (NormalizePath(path) == NormalizePath(m_CurrentDirectory))
            flags |= ImGuiTreeNodeFlags_Selected;

        if (IsOnPathToCurrentDirectory(path))
            flags |= ImGuiTreeNodeFlags_DefaultOpen;

        const bool opened = ImGui::TreeNodeEx(displayName.c_str(), flags);

        if (ImGui::IsItemClicked())
            m_CurrentDirectory = path;

        if (opened)
        {
            for (auto& directoryEntry : std::filesystem::directory_iterator(path))
            {
                if (directoryEntry.is_directory())
                    DrawTree(directoryEntry.path(), assetsPath);
            }
            ImGui::TreePop();
        }
    }
    void ContentBrowserPanel::Refresh()
    {
        m_ImageThumbnailCache.clear();
        if (Project::GetActive())
            m_CurrentDirectory = Project::GetAssetDirectory();
    }

    std::filesystem::path ContentBrowserPanel::ResolveUniqueDestination(
        const std::filesystem::path& destinationDirectory, const std::filesystem::path& fileName) const
    {
        std::filesystem::path destination = destinationDirectory / fileName;
        if (!std::filesystem::exists(destination))
            return destination;

        const std::string stem = fileName.stem().string();
        const std::string extension = fileName.extension().string();

        for (int duplicateIndex = 1; duplicateIndex < 1000; ++duplicateIndex)
        {
            const std::string candidateFileName =
                stem + " (" + std::to_string(duplicateIndex) + ")" + extension;
            destination = destinationDirectory / candidateFileName;
            if (!std::filesystem::exists(destination))
                return destination;
        }

        return destinationDirectory / fileName;
    }

    void ContentBrowserPanel::ImportSingleFile(const std::filesystem::path& sourcePath,
                                               const std::filesystem::path& assetsDirectory)
    {
        if (!std::filesystem::exists(sourcePath) || !std::filesystem::is_regular_file(sourcePath))
            return;

        const std::filesystem::path fileName = sourcePath.filename();
        const std::filesystem::path destinationPath =
            ResolveUniqueDestination(m_CurrentDirectory, fileName);

        try
        {
            std::filesystem::copy_file(sourcePath, destinationPath,
                                       std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::filesystem::filesystem_error& error)
        {
            HIMII_CORE_ERROR("Failed to copy asset file: {0}", error.what());
            return;
        }

        const std::filesystem::path relativePath =
            std::filesystem::relative(destinationPath, assetsDirectory);

        if (fileName.extension() == ".cs")
        {
            if (m_OnScriptChanged)
                m_OnScriptChanged();
            return;
        }

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        AssetHandle handle = assetManager->ImportAsset(relativePath);
        if (handle == 0)
            HIMII_CORE_WARNING("Imported file is not a registered asset type: {0}", destinationPath.string());
    }

    void ContentBrowserPanel::ImportFilesFromPaths(const std::vector<std::filesystem::path>& sourcePaths)
    {
        if (!Project::GetActive() || sourcePaths.empty())
            return;

        const std::filesystem::path assetsDirectory = Project::GetAssetDirectory();

        for (const std::filesystem::path& sourcePath : sourcePaths)
        {
            if (!std::filesystem::exists(sourcePath))
                continue;

            if (std::filesystem::is_directory(sourcePath))
            {
                for (const auto& directoryEntry :
                     std::filesystem::recursive_directory_iterator(sourcePath))
                {
                    if (directoryEntry.is_regular_file())
                        ImportSingleFile(directoryEntry.path(), assetsDirectory);
                }
            }
            else
            {
                ImportSingleFile(sourcePath, assetsDirectory);
            }
        }

        if (auto assetManager = ResourceSystem::GetAssetManager())
            assetManager->SerializeAssetRegistry();
    }

    bool ContentBrowserPanel::CreateCSharpScript(const std::filesystem::path& directory, const std::string& className)
    {
        if (className.empty())
            return false;

        std::filesystem::path scriptPath = directory / (className + ".cs");
        if (std::filesystem::exists(scriptPath))
            return false;

        std::ofstream file(scriptPath);
        if (!file.is_open())
            return false;

        file << "using HimiiEngine;\n\n";
        file << "public class " << className << " : Entity\n";
        file << "{\n";
        file << "    [SerializeField]\n";
        file << "    private float moveSpeed = 1.0f;\n\n";
        file << "\tpublic override void OnCreate()\n";
        file << "\t{\n";
        file << "\t}\n\n";
        file << "\tpublic override void OnUpdate(float ts)\n";
        file << "\t{\n";
        file << "\t}\n";
        file << "}\n";
        file.close();

        return true;
    }
} // namespace Himii
