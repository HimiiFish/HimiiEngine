#pragma once
#include <string>
#include <filesystem>
#include <EngineCore/Core/Core.h>
#include "Resource/AssetManager.h"
#include "Resource/ResourceSystem.h"
#include "EngineCore/Core/Log.h"
#include "Module/Script/ScriptIDE.h"
#include "Project/Physics2DLayerSettings.h"
#include "Project/SortingLayerSettings.h"

namespace Himii
{
	struct ProjectConfig
	{
        std::string Name = "Untitled";
		std::filesystem::path StartScene;
		std::filesystem::path AssetDirectory = "assets";
        std::filesystem::path ScriptModulePath = "bin/Debug/GameAssembly.dll";
        bool Is2D = false;

        ScriptIDEType ScriptIDEOverride = ScriptIDEType::Inherit;
        std::string CustomScriptIDEPath;
        std::string CustomScriptIDEArguments;

        Physics2DLayerSettings Physics2DLayers;
        SortingLayerSettings SortingLayers;
	};

	class Project {
    public:
        static const std::filesystem::path &GetProjectDirectory()
        {
            HIMII_CORE_ASSERT(s_ActiveProject);
            return s_ActiveProject->m_ProjectDirectory;
        }

        static std::filesystem::path GetAssetDirectory()
        {
            HIMII_CORE_ASSERT(s_ActiveProject);
            return GetProjectDirectory() / s_ActiveProject->m_Config.AssetDirectory;
        }

        // TODO(Yan): move to asset manager when we have one
        static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path &path)
        {
            HIMII_CORE_ASSERT(s_ActiveProject);
            return GetAssetDirectory() / path;
        }

        static ProjectConfig &GetConfig()
        {
            return s_ActiveProject->m_Config;
        }

        static Ref<Project> GetActive()
        {
            return s_ActiveProject;
        }

        static Ref<AssetManager> GetAssetManager()
        {
            Ref<AssetManager> boundManager = ResourceSystem::GetAssetManager();
            HIMII_CORE_ASSERT(boundManager);
            return boundManager;
        }

        static Ref<AssetManager> TryGetAssetManager()
        {
            return ResourceSystem::GetAssetManager();
        }

        static std::filesystem::path GetAssetRegistryPath()
        {
            HIMII_CORE_ASSERT(s_ActiveProject);
            // 保存为 ProjectDir/AssetRegistry.yaml
            return s_ActiveProject->m_ProjectDirectory / "AssetRegistry.yaml";
        }

        static Ref<Project> New();
        static Ref<Project> Load(const std::filesystem::path &path);
        static bool SaveActive(const std::filesystem::path &path);

        static void CreateProjectFiles(const std::string &name, const std::filesystem::path &projectDir);

        /// 将 ScriptCore.dll / .pdb / .xml 与 Himii 脚本 API 源文件同步到项目目录，供 IDE 智能提示解析
        static void SyncScriptCoreToProjectDirectory(const std::filesystem::path &projectDir);

        /// 缺省时从引擎内容拷贝默认字体（及 3D 天空盒）到项目 assets，并登记 AssetRegistry。
        static void EnsureSeededDefaultAssets();

        /// 玩法默认字体路径（相对 assets）：fonts/msyh.ttc
        static std::filesystem::path GetDefaultGameplayFontRelativePath();

        /// 从项目 assets 初始化玩法默认字体；缺文件则告警且不回退引擎包。
        static void InitializeGameplayDefaultFont();

    private:
        ProjectConfig m_Config;
        std::filesystem::path m_ProjectDirectory;
        Ref<AssetManager> m_AssetManager;

        inline static Ref<Project> s_ActiveProject;
        friend class ProjectSerializer; 
    };
}
