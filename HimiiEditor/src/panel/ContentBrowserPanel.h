#pragma once
#include "Resource/Asset.h"
#include "Module/Render/RenderCore/Texture.h"

#include <array>
#include <filesystem>
#include <functional>
#include <unordered_map>

namespace Himii
{
    class ContentBrowserPanel {
    public:
        ContentBrowserPanel();

        void OnImGuiRender();

        void Refresh();
        void SetOnScriptChanged(std::function<void()> callback) { m_OnScriptChanged = std::move(callback); }

        /// 从操作系统拖入或文件对话框导入资源到当前目录
        void ImportFilesFromPaths(const std::vector<std::filesystem::path>& sourcePaths);

        AssetHandle GetTextureInspectorRequest()
        {
            AssetHandle handle = m_TextureInspectorRequest;
            m_TextureInspectorRequest = 0;
            return handle;
        }

        AssetHandle GetMaterialEditorRequest()
        {
            AssetHandle handle = m_MaterialEditorRequest;
            m_MaterialEditorRequest = 0;
            return handle;
        }

        std::filesystem::path GetAnimationEditorRequest()
        {
            std::filesystem::path path = m_AnimationEditorRequest;
            m_AnimationEditorRequest.clear();
            return path;
        }

    private:
        enum class CreationType
        {
            None = 0,
            Folder,
            CSharpScript,
            Scene,
            Material,
            ParticleEmitter,
            SpriteAnimation,
            TileMap,
            TileSet
        };

        void DrawCreateMenu(const std::filesystem::path &targetDirectory);
        void BeginCreation(CreationType creationType, const std::filesystem::path &targetDirectory,
                           const char *defaultName);
        void DrawCreationModal();
        bool CreatePendingItem();
        bool ValidateCreationName(const std::string &name, std::string &errorMessage) const;
        bool IsPathInsideAssetsDirectory(const std::filesystem::path &path) const;
        bool CreateFolder(const std::filesystem::path &directory, const std::string &folderName);
        bool CreateCSharpScript(const std::filesystem::path& directory, const std::string& className);
        bool CreateSceneAsset(const std::filesystem::path &directory, const std::string &sceneName);
        bool CreateMaterialAsset(const std::filesystem::path &directory, const std::string &materialName);
        bool CreateParticleEmitterAsset(const std::filesystem::path &directory,
                                        const std::string &emitterName);
        bool CreateSpriteAnimationAsset(const std::filesystem::path &directory,
                                        const std::string &animationName);
        bool CreateTileSetAsset(const std::filesystem::path &directory,
                                const std::string &tileSetName);
        bool CreateTileMapAssetPair(const std::filesystem::path &directory,
                                    const std::string &tileMapName);
        AssetHandle RegisterCreatedAsset(const std::filesystem::path &absolutePath,
                                         bool persistRegistry = true);
        void ImportSingleFile(const std::filesystem::path& sourcePath,const std::filesystem::path& assetsDirectory);
        std::filesystem::path ResolveUniqueDestination(const std::filesystem::path& destinationDirectory,  const std::filesystem::path& fileName) const;
        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;

        Ref<Texture2D> m_DirectoryIcon;
        Ref<Texture2D> m_FileIcon;
        Ref<Texture2D> m_ScriptIcon;
        Ref<Texture2D> m_SceneIcon;
        
        void DrawTree(const std::filesystem::path& path, const std::filesystem::path& assetsPath);
        void DrawContentDetailBar(float barWidth);
        bool IsOnPathToCurrentDirectory(const std::filesystem::path& path) const;
        static std::string TruncateTextToWidth(const char* text, float maxWidth);
        static bool ShouldHideFromContentBrowser(const std::filesystem::path& path);
        Ref<Texture2D> GetOrLoadImageThumbnail(const std::filesystem::path& relativePath);

        std::unordered_map<std::string, Ref<Texture2D>> m_ImageThumbnailCache;

        std::function<void()> m_OnScriptChanged;
        std::string m_SelectedItemDisplayName;
        bool m_ScrollToSelectedItem = false;
        std::filesystem::path m_LastBrowsedDirectory;
        AssetHandle m_TextureInspectorRequest = 0;
        AssetHandle m_MaterialEditorRequest = 0;
        std::filesystem::path m_AnimationEditorRequest;
        CreationType m_PendingCreationType = CreationType::None;
        std::filesystem::path m_CreationTargetDirectory;
        std::array<char, 128> m_CreationNameBuffer{};
        std::string m_CreationErrorMessage;
        bool m_OpenCreationModal = false;
    };
}
