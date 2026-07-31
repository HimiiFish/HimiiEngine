#pragma once
#include "Resource/Asset.h"
#include "Module/Render/Texture.h"

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

        std::filesystem::path GetAnimationEditorRequest()
        {
            std::filesystem::path path = m_AnimationEditorRequest;
            m_AnimationEditorRequest.clear();
            return path;
        }

    private:
        bool CreateCSharpScript(const std::filesystem::path& directory, const std::string& className);
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
        std::filesystem::path m_LastBrowsedDirectory;
        AssetHandle m_TextureInspectorRequest = 0;
        std::filesystem::path m_AnimationEditorRequest;
    };
}
