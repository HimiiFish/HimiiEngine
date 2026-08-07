#include "Hepch.h"
#include "ContentBrowserPanel.h"
#include "EditorExternalFileDrop.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "Resource/AssetManager.h"
#include "Resource/SpriteSheetUtility.h"
#include "Module/Render/Mesh/MaterialAsset.h"
#include "Module/Render/Mesh/MaterialAssetSerializer.h"
#include "Module/Render/Shader/BuiltinShaderRegistry.h"
#include "Module/Render/Shader/ShaderAsset.h"
#include "Module/Render/Shader/ShaderAssetSerializer.h"
#include "Module/Script/ScriptIDELauncher.h"
#include "panel/MaterialThumbnailUtility.h"
#include "Module/Render/Mesh/MeshAssetSerializer.h"
#include "Module/Render/Mesh/StaticMeshImporter.h"
#include "Module/Render/Mesh/MeshCompanionImport.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include "Module/Particle/ParticleEmitterAssetSerializer.h"
#include "Module/Animation/SpriteAnimationSerializer.h"
#include "Module/Tilemap/TileMapDataSerializer.h"
#include "Module/Tilemap/TileSetSerializer.h"
#include "World/Scene/Entity.h"
#include "World/Scene/Scene.h"
#include "World/Scene/SceneSerializer.h"
#include "World/Scene/Components.h"
#include "EngineCore/Utils/PlatformUtils.h"
#include "EngineCore/Core/Log.h"

#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <unordered_set>
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
        if (extension == ".meta")
            return true;
        return IsStaticMeshSourceExtension(extension);
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

        if (auto assetManager = ResourceSystem::GetAssetManager())
            ProcessPendingMaterialThumbnails(assetManager.get(), 2);

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

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    (void)payload;
                }
                ImGui::EndDragDropTarget();
            }

            std::vector<std::filesystem::path> droppedPaths;
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
            {
                droppedPaths = EditorExternalFileDrop::ConsumePendingPaths();
                if (!droppedPaths.empty())
                    ImportFilesFromPaths(droppedPaths);
            }

            if (NormalizePath(m_LastBrowsedDirectory) != NormalizePath(m_CurrentDirectory))
            {
                m_SelectedItemDisplayName.clear();
                m_ScrollToSelectedItem = false;
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
                    else if (path.extension() == ".hmaterial")
                    {
                        if (auto assetManager = ResourceSystem::GetAssetManager())
                        {
                            AssetHandle materialHandle =
                                    assetManager->FindAssetHandleByFilePath(relativePath);
                            if (materialHandle == 0)
                                materialHandle = assetManager->ImportAsset(relativePath);
                            if (materialHandle != 0)
                                imageThumbnail =
                                        GetOrCreateMaterialThumbnail(assetManager.get(), materialHandle);
                        }
                    }

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
                        else if (path.extension() == ".hmaterial")
                        {
                            if (auto assetManager = ResourceSystem::GetAssetManager())
                            {
                                const AssetHandle materialHandle = assetManager->ImportAsset(relativePath);
                                if (materialHandle != 0)
                                    m_MaterialEditorRequest = materialHandle;
                            }
                        }
                        else if (path.extension() == ".hshader")
                        {
                            OpenShaderAssetInIde(Project::GetAssetFileSystemPath(relativePath));
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

                    if (isItemSelected && m_ScrollToSelectedItem)
                    {
                        ImGui::SetScrollHereY(0.5f);
                        m_ScrollToSelectedItem = false;
                    }

                    if (directoryEntry.is_directory()
                        && ImGui::BeginPopupContextItem("DirectoryContext"))
                    {
                        DrawCreateMenu(path);
                        ImGui::EndPopup();
                    }
                    else if (!directoryEntry.is_directory())
                    {
                        std::string fileExtension = path.extension().string();
                        std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(),
                                       [](unsigned char character)
                                       { return static_cast<char>(std::tolower(character)); });
                        if (fileExtension == ".hmesh"
                            && ImGui::BeginPopupContextItem("StaticMeshProductContext"))
                        {
                            if (ImGui::MenuItem("Reimport"))
                                BeginStaticMeshReimport(relativePath);
                            ImGui::EndPopup();
                        }
                        else if (fileExtension == ".hshader"
                                 && ImGui::BeginPopupContextItem("ShaderAssetContext"))
                        {
                            if (ImGui::MenuItem("Open in IDE"))
                                OpenShaderAssetInIde(Project::GetAssetFileSystemPath(relativePath));
                            if (ImGui::MenuItem("Create Material From Shader"))
                                BeginMaterialCreationFromShader(relativePath);
                            ImGui::EndPopup();
                        }
                    }

                    ImGui::PopID();

                    column++;
                    if (column >= columnCount)
                        column = 0;
                }

                // The grid lives in its own child window, so the blank-area context menu must be
                // opened here; attaching it to the parent window would only cover the detail bar.
                if (ImGui::BeginPopupContextWindow("ContentBrowserContext",
                                                   ImGuiPopupFlags_MouseButtonRight
                                                           | ImGuiPopupFlags_NoOpenOverItems))
                {
                    DrawCreateMenu(m_CurrentDirectory);
                    if (ImGui::MenuItem("Import Asset..."))
                    {
                        std::string selectedPath = FileDialog::OpenFile(
                            "Static Mesh Sources (*.glb;*.gltf;*.fbx;*.obj)\0*.glb;*.gltf;*.fbx;*.obj\0"
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
            }
            ImGui::EndChild();

            DrawContentDetailBar(contentColumnWidth);

            ImGui::EndTable();
        }

        DrawCreationModal();
        DrawStaticMeshImportDialogIfNeeded();
        ImGui::End();
    }

    void ContentBrowserPanel::DrawCreateMenu(const std::filesystem::path &targetDirectory)
    {
        if (!ImGui::BeginMenu("Create"))
            return;

        if (ImGui::MenuItem("Folder"))
            BeginCreation(CreationType::Folder, targetDirectory, "New Folder");
        if (ImGui::MenuItem("C# Script"))
            BeginCreation(CreationType::CSharpScript, targetDirectory, "NewScript");
        if (ImGui::MenuItem("Scene"))
            BeginCreation(CreationType::Scene, targetDirectory, "New Scene");

        if (ImGui::BeginMenu("Rendering"))
        {
            if (ImGui::MenuItem("Shader"))
                BeginCreation(CreationType::Shader, targetDirectory, "NewShader");
            if (ImGui::MenuItem("Material"))
                BeginMaterialCreationWithDefaultShader(targetDirectory);
            if (ImGui::MenuItem("Particle Emitter"))
                BeginCreation(CreationType::ParticleEmitter, targetDirectory, "New Particle Emitter");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Animation"))
        {
            if (ImGui::MenuItem("Sprite Animation"))
                BeginCreation(CreationType::SpriteAnimation, targetDirectory, "New Animation");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tilemap"))
        {
            if (ImGui::MenuItem("Tile Map"))
                BeginCreation(CreationType::TileMap, targetDirectory, "New Tile Map");
            if (ImGui::MenuItem("Tile Set"))
                BeginCreation(CreationType::TileSet, targetDirectory, "New Tile Set");
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    void ContentBrowserPanel::BeginCreation(CreationType creationType,
                                            const std::filesystem::path &targetDirectory,
                                            const char *defaultName)
    {
        m_PendingCreationType = creationType;
        m_CreationTargetDirectory = targetDirectory;
        m_CreationNameBuffer.fill('\0');
        if (defaultName)
        {
            const size_t characterCount =
                    std::min(std::strlen(defaultName), m_CreationNameBuffer.size() - 1);
            std::copy_n(defaultName, characterCount, m_CreationNameBuffer.begin());
        }
        m_CreationErrorMessage.clear();
        m_OpenCreationModal = true;
    }

    void ContentBrowserPanel::DrawCreationModal()
    {
        if (m_OpenCreationModal)
        {
            ImGui::OpenPopup("Create Asset");
            m_OpenCreationModal = false;
        }

        if (!ImGui::BeginPopupModal("Create Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        ImGui::TextUnformatted("Name");
        ImGui::SetNextItemWidth(320.0f);
        const bool submittedByKeyboard = ImGui::InputText(
                "##CreationName", m_CreationNameBuffer.data(), m_CreationNameBuffer.size(),
                ImGuiInputTextFlags_EnterReturnsTrue);

        if (!m_CreationErrorMessage.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.35f, 0.3f, 1.0f));
            ImGui::TextWrapped("%s", m_CreationErrorMessage.c_str());
            ImGui::PopStyleColor();
        }

        const bool createRequested = submittedByKeyboard || ImGui::Button("Create");
        if (createRequested)
        {
            if (CreatePendingItem())
            {
                m_PendingCreationType = CreationType::None;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_PendingCreationType = CreationType::None;
            m_CreationErrorMessage.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    bool ContentBrowserPanel::ValidateCreationName(const std::string &name,
                                                   std::string &errorMessage) const
    {
        if (name.empty())
        {
            errorMessage = "Name cannot be empty.";
            return false;
        }
        if (name == "." || name == "..")
        {
            errorMessage = "This name is reserved.";
            return false;
        }
        if (std::isspace(static_cast<unsigned char>(name.front()))
            || std::isspace(static_cast<unsigned char>(name.back()))
            || name.back() == '.')
        {
            errorMessage = "Name cannot start or end with whitespace or a period.";
            return false;
        }

        constexpr const char *invalidCharacters = "<>:\"/\\|?*";
        if (name.find_first_of(invalidCharacters) != std::string::npos
            || std::any_of(name.begin(), name.end(), [](unsigned char character)
                           {
                               return character < 32;
                           }))
        {
            errorMessage = "Name contains characters that are invalid on Windows.";
            return false;
        }

        std::string uppercaseName(name);
        std::transform(uppercaseName.begin(), uppercaseName.end(), uppercaseName.begin(),
                       [](unsigned char character)
                       {
                           return static_cast<char>(std::toupper(character));
                       });
        static const std::unordered_set<std::string> reservedWindowsNames = {
                "CON", "PRN", "AUX", "NUL",
                "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
        if (reservedWindowsNames.find(uppercaseName) != reservedWindowsNames.end())
        {
            errorMessage = "This name is reserved by Windows.";
            return false;
        }

        if (m_PendingCreationType == CreationType::CSharpScript)
        {
            const auto validIdentifierStart = [](unsigned char character)
            {
                return std::isalpha(character) || character == '_';
            };
            const auto validIdentifierCharacter = [](unsigned char character)
            {
                return std::isalnum(character) || character == '_';
            };
            if (!validIdentifierStart(static_cast<unsigned char>(name.front()))
                || !std::all_of(name.begin() + 1, name.end(), validIdentifierCharacter))
            {
                errorMessage = "C# class names may contain only letters, digits, and underscores, "
                               "and cannot start with a digit.";
                return false;
            }

            static const std::unordered_set<std::string> cSharpKeywords = {
                    "abstract", "as", "base", "bool", "break", "byte", "case", "catch",
                    "char", "checked", "class", "const", "continue", "decimal", "default",
                    "delegate", "do", "double", "else", "enum", "event", "explicit", "extern",
                    "false", "finally", "fixed", "float", "for", "foreach", "goto", "if",
                    "implicit", "in", "int", "interface", "internal", "is", "lock", "long",
                    "namespace", "new", "null", "object", "operator", "out", "override",
                    "params", "private", "protected", "public", "readonly", "ref", "return",
                    "sbyte", "sealed", "short", "sizeof", "stackalloc", "static", "string",
                    "struct", "switch", "this", "throw", "true", "try", "typeof", "uint",
                    "ulong", "unchecked", "unsafe", "ushort", "using", "virtual", "void",
                    "volatile", "while"};
            if (cSharpKeywords.find(name) != cSharpKeywords.end())
            {
                errorMessage = "This name is a reserved C# keyword.";
                return false;
            }
        }

        return true;
    }

    bool ContentBrowserPanel::IsPathInsideAssetsDirectory(const std::filesystem::path &path) const
    {
        if (!Project::GetActive())
            return false;

        const std::filesystem::path normalizedAssetsDirectory =
                NormalizePath(Project::GetAssetDirectory());
        const std::filesystem::path normalizedPath = NormalizePath(path);
        std::error_code errorCode;
        const std::filesystem::path relativePath =
                std::filesystem::relative(normalizedPath, normalizedAssetsDirectory, errorCode);
        if (errorCode || relativePath.empty())
            return normalizedPath == normalizedAssetsDirectory;

        const std::string relativeString = relativePath.generic_string();
        return relativeString != ".." && relativeString.rfind("../", 0) != 0;
    }

    bool ContentBrowserPanel::CreatePendingItem()
    {
        m_CreationErrorMessage.clear();
        const std::string itemName = m_CreationNameBuffer.data();
        if (!ValidateCreationName(itemName, m_CreationErrorMessage))
            return false;
        if (!std::filesystem::is_directory(m_CreationTargetDirectory)
            || !IsPathInsideAssetsDirectory(m_CreationTargetDirectory))
        {
            m_CreationErrorMessage = "The target directory is outside the active project's Assets directory.";
            return false;
        }

        std::filesystem::path expectedItemPath;
        switch (m_PendingCreationType)
        {
            case CreationType::Folder:
                expectedItemPath = m_CreationTargetDirectory / itemName;
                break;
            case CreationType::CSharpScript:
                expectedItemPath = m_CreationTargetDirectory / (itemName + ".cs");
                break;
            case CreationType::Scene:
                expectedItemPath = m_CreationTargetDirectory / (itemName + ".himii");
                break;
            case CreationType::Material:
                expectedItemPath = m_CreationTargetDirectory / (itemName + ".hmaterial");
                break;
            case CreationType::Shader:
                expectedItemPath = m_CreationTargetDirectory / (itemName + ".hshader");
                break;
            case CreationType::ParticleEmitter:
                expectedItemPath = m_CreationTargetDirectory / (itemName + ".particle");
                break;
            case CreationType::SpriteAnimation:
                expectedItemPath = m_CreationTargetDirectory / (itemName + ".anim");
                break;
            case CreationType::TileMap:
                expectedItemPath = m_CreationTargetDirectory / (itemName + ".tilemap");
                break;
            case CreationType::TileSet:
                expectedItemPath = m_CreationTargetDirectory / (itemName + ".tileset");
                break;
            default:
                break;
        }
        if (!expectedItemPath.empty() && std::filesystem::exists(expectedItemPath))
        {
            m_CreationErrorMessage = "An item with this name already exists.";
            return false;
        }
        if (m_PendingCreationType == CreationType::TileMap)
        {
            const std::filesystem::path expectedTileSetPath =
                    m_CreationTargetDirectory / (itemName + ".tileset");
            if (std::filesystem::exists(expectedTileSetPath))
            {
                m_CreationErrorMessage = "An item with this name already exists.";
                return false;
            }
        }

        bool created = false;
        std::string createdFileName;
        switch (m_PendingCreationType)
        {
            case CreationType::Folder:
                created = CreateFolder(m_CreationTargetDirectory, itemName);
                createdFileName = itemName;
                break;
            case CreationType::CSharpScript:
                created = CreateCSharpScript(m_CreationTargetDirectory, itemName);
                createdFileName = itemName + ".cs";
                break;
            case CreationType::Scene:
                created = CreateSceneAsset(m_CreationTargetDirectory, itemName);
                createdFileName = itemName + ".himii";
                break;
            case CreationType::Material:
                created = CreateMaterialAsset(m_CreationTargetDirectory, itemName, m_PendingCreationShaderHandle);
                createdFileName = itemName + ".hmaterial";
                break;
            case CreationType::Shader:
                created = CreateShaderAsset(m_CreationTargetDirectory, itemName);
                createdFileName = itemName + ".hshader";
                break;
            case CreationType::ParticleEmitter:
                created = CreateParticleEmitterAsset(m_CreationTargetDirectory, itemName);
                createdFileName = itemName + ".particle";
                break;
            case CreationType::SpriteAnimation:
                created = CreateSpriteAnimationAsset(m_CreationTargetDirectory, itemName);
                createdFileName = itemName + ".anim";
                break;
            case CreationType::TileMap:
                created = CreateTileMapAssetPair(m_CreationTargetDirectory, itemName);
                createdFileName = itemName + ".tilemap";
                break;
            case CreationType::TileSet:
                created = CreateTileSetAsset(m_CreationTargetDirectory, itemName);
                createdFileName = itemName + ".tileset";
                break;
            case CreationType::None:
            default:
                m_CreationErrorMessage = "No creation type was selected.";
                return false;
        }

        if (!created)
        {
            if (m_CreationErrorMessage.empty())
                m_CreationErrorMessage = "An item with this name already exists or could not be created.";
            return false;
        }

        m_SelectedItemDisplayName = createdFileName;
        m_ScrollToSelectedItem = true;
        m_PendingCreationShaderHandle = 0;
        if (m_PendingCreationType == CreationType::CSharpScript && m_OnScriptChanged)
            m_OnScriptChanged();
        return true;
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
        ClearMaterialThumbnailCache();
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

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        std::error_code errorCode;
        const bool sourceInsideAssets = IsPathInsideAssetsDirectory(sourcePath);

        std::filesystem::path destinationPath;
        std::filesystem::path relativePath;
        if (sourceInsideAssets)
        {
            // Already under Assets: import in place without copying a duplicate.
            destinationPath = sourcePath;
            relativePath = std::filesystem::relative(destinationPath, assetsDirectory, errorCode);
            if (errorCode)
            {
                HIMII_CORE_ERROR("Failed to resolve asset-relative path for: {0}", sourcePath.string());
                return;
            }
        }
        else
        {
            const std::filesystem::path fileName = sourcePath.filename();
            destinationPath = ResolveUniqueDestination(m_CurrentDirectory, fileName);

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

            relativePath = std::filesystem::relative(destinationPath, assetsDirectory, errorCode);
            if (errorCode)
            {
                HIMII_CORE_ERROR("Failed to resolve asset-relative path for: {0}", destinationPath.string());
                return;
            }
        }

        relativePath = std::filesystem::path(relativePath.generic_string());

        if (destinationPath.extension() == ".cs")
        {
            if (m_OnScriptChanged)
                m_OnScriptChanged();
            return;
        }

        std::string fileExtension = destinationPath.extension().string();
        std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(),
                       [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        if (IsStaticMeshSourceExtension(fileExtension))
        {
            BeginStaticMeshSourceImport(relativePath);
            return;
        }

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

    AssetHandle ContentBrowserPanel::RegisterCreatedAsset(const std::filesystem::path &absolutePath,
                                                        bool persistRegistry)
    {
        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || !Project::GetActive())
            return 0;

        std::error_code errorCode;
        const std::filesystem::path relativePath =
                std::filesystem::relative(absolutePath, Project::GetAssetDirectory(), errorCode);
        if (errorCode)
            return 0;

        const AssetHandle assetHandle = assetManager->ImportAsset(relativePath);
        if (assetHandle != 0 && persistRegistry)
            assetManager->SerializeAssetRegistry();
        return assetHandle;
    }

    bool ContentBrowserPanel::CreateFolder(const std::filesystem::path &directory,
                                           const std::string &folderName)
    {
        const std::filesystem::path folderPath = directory / folderName;
        if (std::filesystem::exists(folderPath))
            return false;

        std::error_code errorCode;
        const bool created = std::filesystem::create_directory(folderPath, errorCode);
        return created && !errorCode;
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
        file << "\tpublic override void OnUpdate(float timestep)\n";
        file << "\t{\n";
        file << "\t}\n";
        file << "}\n";
        file.close();

        return true;
    }

    bool ContentBrowserPanel::CreateSceneAsset(const std::filesystem::path &directory,
                                               const std::string &sceneName)
    {
        if (!Project::GetActive())
            return false;

        const std::filesystem::path scenePath = directory / (sceneName + ".himii");
        if (std::filesystem::exists(scenePath))
            return false;

        Ref<Scene> scene = CreateRef<Scene>();
        Entity cameraEntity = scene->CreateEntity("Main Camera");
        auto &cameraComponent = cameraEntity.AddComponent<CameraComponent>();
        cameraEntity.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, 0.0f, 10.0f);

        if (Project::GetActive()->GetConfig().Is2D)
        {
            cameraComponent.Camera.SetProjectionType(SceneCamera::ProjectionType::Orthographic);
        }
        else
        {
            cameraComponent.Camera.SetProjectionType(SceneCamera::ProjectionType::Perspective);

            Entity lightEntity = scene->CreateEntity("Directional Light");
            lightEntity.GetComponent<TransformComponent>().Rotation =
                    glm::radians(glm::vec3(50.0f, -30.0f, 0.0f));
            lightEntity.AddComponent<LightComponent>();

            Entity environmentEntity = scene->CreateEntity("Environment");
            environmentEntity.AddComponent<EnvironmentComponent>();
        }

        SceneSerializer sceneSerializer(scene);
        sceneSerializer.Serialize(scenePath.string());
        RegisterCreatedAsset(scenePath);
        return std::filesystem::exists(scenePath);
    }

    bool ContentBrowserPanel::CreateMaterialAsset(const std::filesystem::path &directory,
                                                  const std::string &materialName,
                                                  AssetHandle shaderHandle)
    {
        if (materialName.empty() || !Project::GetActive())
            return false;

        const std::filesystem::path absoluteMaterialPath = directory / (materialName + ".hmaterial");
        if (std::filesystem::exists(absoluteMaterialPath))
            return false;

        if (shaderHandle == 0)
            shaderHandle = BuiltinShaderRegistry::GetDefaultLitShaderHandle();

        Ref<MaterialAsset> materialAsset =
                MaterialAssetSerializer::CreateDefaultMaterialInstance(shaderHandle);
        MaterialAssetSerializer::Serialize(absoluteMaterialPath, materialAsset);

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return false;

        const std::filesystem::path relativeMaterialPath =
                std::filesystem::relative(absoluteMaterialPath, Project::GetAssetDirectory());
        const AssetHandle materialHandle = assetManager->ImportAsset(relativeMaterialPath.generic_string());
        if (materialHandle == 0)
            return false;

        materialAsset->Handle = materialHandle;
        MaterialAssetSerializer::Serialize(absoluteMaterialPath, materialAsset);
        assetManager->SerializeAssetRegistry();
        return true;
    }

    bool ContentBrowserPanel::CreateShaderAsset(const std::filesystem::path &directory,
                                                const std::string &shaderName)
    {
        if (shaderName.empty() || !Project::GetActive())
            return false;

        const std::filesystem::path absoluteShaderPath = directory / (shaderName + ".hshader");
        if (std::filesystem::exists(absoluteShaderPath))
            return false;

        Ref<ShaderAsset> shaderAsset = CreateRef<ShaderAsset>();
        shaderAsset->PipelineType = ShaderPipelineType::SpatialLit;
        shaderAsset->PropertyDefinitions = BuiltinShaderRegistry::GetMeshLitPropertyDefinitions();
        shaderAsset->SourceCode = ShaderAssetSerializer::BuildDefaultSpatialLitTemplate();
        ShaderAssetSerializer::Serialize(absoluteShaderPath, shaderAsset);

        RegisterCreatedAsset(absoluteShaderPath);
        return std::filesystem::exists(absoluteShaderPath);
    }

    void ContentBrowserPanel::OpenShaderAssetInIde(const std::filesystem::path &absoluteShaderPath)
    {
        if (!Project::GetActive() || absoluteShaderPath.empty())
            return;

        ScriptIDELauncher::OpenScript(Project::GetProjectDirectory(), Project::GetConfig().Name,
                                      absoluteShaderPath);
    }

    void ContentBrowserPanel::BeginMaterialCreationWithDefaultShader(
            const std::filesystem::path &targetDirectory)
    {
        m_PendingCreationShaderHandle = BuiltinShaderRegistry::GetDefaultLitShaderHandle();
        BeginCreation(CreationType::Material, targetDirectory, "NewMaterial");
    }

    void ContentBrowserPanel::BeginMaterialCreationFromShader(
            const std::filesystem::path &relativeShaderPath)
    {
        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        const AssetHandle shaderHandle = assetManager->ImportAsset(relativeShaderPath);
        if (shaderHandle == 0)
            return;

        m_PendingCreationShaderHandle = shaderHandle;
        const std::string defaultMaterialName = relativeShaderPath.stem().string() + "_Material";
        BeginCreation(CreationType::Material, relativeShaderPath.parent_path(),
                      defaultMaterialName.c_str());
    }

    bool ContentBrowserPanel::CreateParticleEmitterAsset(const std::filesystem::path &directory,
                                                         const std::string &emitterName)
    {
        const std::filesystem::path emitterPath = directory / (emitterName + ".particle");
        if (std::filesystem::exists(emitterPath))
            return false;

        Ref<ParticleEmitterAsset> emitterAsset = CreateRef<ParticleEmitterAsset>();
        ParticleEmitterAssetSerializer::Serialize(emitterPath, emitterAsset);
        const AssetHandle emitterHandle = RegisterCreatedAsset(emitterPath);
        if (emitterHandle != 0)
        {
            emitterAsset->Handle = emitterHandle;
            ParticleEmitterAssetSerializer::Serialize(emitterPath, emitterAsset);
        }
        return std::filesystem::exists(emitterPath);
    }

    bool ContentBrowserPanel::CreateSpriteAnimationAsset(const std::filesystem::path &directory,
                                                         const std::string &animationName)
    {
        const std::filesystem::path animationPath = directory / (animationName + ".anim");
        if (std::filesystem::exists(animationPath))
            return false;

        Ref<SpriteAnimation> animationAsset = CreateRef<SpriteAnimation>();
        animationAsset->EnsureClip(SpriteAnimationDefaultClipName);
        SpriteAnimationSerializer::Serialize(animationPath, animationAsset);
        const AssetHandle animationHandle = RegisterCreatedAsset(animationPath);
        if (animationHandle != 0)
        {
            animationAsset->Handle = animationHandle;
            SpriteAnimationSerializer::Serialize(animationPath, animationAsset);
        }
        return std::filesystem::exists(animationPath);
    }

    bool ContentBrowserPanel::CreateTileSetAsset(const std::filesystem::path &directory,
                                                 const std::string &tileSetName)
    {
        const std::filesystem::path tileSetPath = directory / (tileSetName + ".tileset");
        if (std::filesystem::exists(tileSetPath))
            return false;

        Ref<TileSet> tileSetAsset = CreateRef<TileSet>();
        TileAtlasSource atlasSource;
        atlasSource.TileSize = 16;
        tileSetAsset->AddAtlasSource(atlasSource);
        TileSetSerializer::Serialize(tileSetPath, tileSetAsset);
        const AssetHandle tileSetHandle = RegisterCreatedAsset(tileSetPath);
        if (tileSetHandle != 0)
        {
            tileSetAsset->Handle = tileSetHandle;
            TileSetSerializer::Serialize(tileSetPath, tileSetAsset);
        }
        return std::filesystem::exists(tileSetPath);
    }

    bool ContentBrowserPanel::CreateTileMapAssetPair(const std::filesystem::path &directory,
                                                     const std::string &tileMapName)
    {
        const std::filesystem::path tileSetPath = directory / (tileMapName + ".tileset");
        const std::filesystem::path tileMapPath = directory / (tileMapName + ".tilemap");
        if (std::filesystem::exists(tileSetPath) || std::filesystem::exists(tileMapPath))
            return false;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return false;

        Ref<TileSet> tileSetAsset = CreateRef<TileSet>();
        TileAtlasSource atlasSource;
        atlasSource.TileSize = 16;
        tileSetAsset->AddAtlasSource(atlasSource);
        TileSetSerializer::Serialize(tileSetPath, tileSetAsset);
        const AssetHandle tileSetHandle = RegisterCreatedAsset(tileSetPath, false);
        if (tileSetHandle == 0)
        {
            std::error_code errorCode;
            std::filesystem::remove(tileSetPath, errorCode);
            return false;
        }
        tileSetAsset->Handle = tileSetHandle;
        TileSetSerializer::Serialize(tileSetPath, tileSetAsset);

        Ref<TileMapData> tileMapAsset = CreateRef<TileMapData>();
        tileMapAsset->SetTileSetHandle(tileSetHandle);
        tileMapAsset->SetCellSize(1.0f);
        TileMapDataSerializer::Serialize(tileMapPath, tileMapAsset);
        const AssetHandle tileMapHandle = RegisterCreatedAsset(tileMapPath, false);
        if (tileMapHandle == 0)
        {
            std::error_code errorCode;
            std::filesystem::remove(tileMapPath, errorCode);
            std::filesystem::remove(tileSetPath, errorCode);
            assetManager->DeserializeAssetRegistry();
            return false;
        }
        tileMapAsset->Handle = tileMapHandle;
        TileMapDataSerializer::Serialize(tileMapPath, tileMapAsset);
        assetManager->SerializeAssetRegistry();
        return true;
    }

    void ContentBrowserPanel::BeginStaticMeshSourceImport(const std::filesystem::path &relativeSourcePath)
    {
        m_StaticMeshImportDialogState = {};
        m_StaticMeshImportDialogState.Open = true;
        m_StaticMeshImportDialogState.IsReimport = false;
        m_StaticMeshImportDialogState.RelativeSourcePath = relativeSourcePath;
        ImGui::OpenPopup("Import Static Mesh");
    }

    void ContentBrowserPanel::BeginStaticMeshReimport(const std::filesystem::path &relativeHmeshPath)
    {
        m_StaticMeshImportDialogState = {};
        m_StaticMeshImportDialogState.Open = true;
        m_StaticMeshImportDialogState.IsReimport = true;
        m_StaticMeshImportDialogState.RelativeHmeshPath = relativeHmeshPath;

        const std::filesystem::path absoluteHmeshPath =
                Project::GetAssetFileSystemPath(relativeHmeshPath);
        StaticMeshImportSettings savedSettings;
        std::filesystem::path relativeSourcePath;
        std::vector<AssetHandle> materialHandles;
        std::vector<std::string> materialSlotNames;
        if (MeshAssetSerializer::ReadStaticMeshMeta(absoluteHmeshPath, savedSettings, relativeSourcePath,
                                                    materialHandles, materialSlotNames))
        {
            m_StaticMeshImportDialogState.Settings = savedSettings;
            m_StaticMeshImportDialogState.RelativeSourcePath = relativeSourcePath;
            m_StaticMeshImportDialogState.HasExistingCompanionMaterials =
                    !relativeSourcePath.empty()
                    && MeshCompanionMaterialsExistOnDisk(relativeSourcePath);
        }

        ImGui::OpenPopup("Import Static Mesh");
    }

    void ContentBrowserPanel::DrawStaticMeshImportDialogIfNeeded()
    {
        if (m_StaticMeshImportDialogState.Open && !ImGui::IsPopupOpen("Import Static Mesh"))
            ImGui::OpenPopup("Import Static Mesh");

        if (!DrawStaticMeshImportDialog(m_StaticMeshImportDialogState))
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        if (m_StaticMeshImportDialogState.IsReimport)
        {
            StaticMeshImporter::ReimportProduct(
                    *assetManager, m_StaticMeshImportDialogState.RelativeHmeshPath,
                    &m_StaticMeshImportDialogState.Settings,
                    m_StaticMeshImportDialogState.PreserveCompanionMaterialsOnReimport);
        }
        else
        {
            StaticMeshImporter::ImportFromSource(
                    *assetManager, m_StaticMeshImportDialogState.RelativeSourcePath,
                    m_StaticMeshImportDialogState.Settings);
        }

        m_ImageThumbnailCache.clear();
    }
} // namespace Himii