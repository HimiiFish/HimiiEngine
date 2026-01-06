#pragma once
#include <string>
#include <filesystem>
#include <Himii/Core/Core.h>
#include "Himii/Asset/AssetManager.h" 
#include "Himii/Core/Log.h"

namespace Himii
{
	struct ProjectConfig
	{
        std::string Name = "Untitled";
		std::filesystem::path StartScene;
		std::filesystem::path AssetDirectory = "assets";
        std::filesystem::path ScriptModulePath = "bin/Debug/GameAssembly.dll";
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
            HIMII_CORE_ASSERT(s_ActiveProject);
            return s_ActiveProject->m_AssetManager;
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

    private:
        ProjectConfig m_Config;
        std::filesystem::path m_ProjectDirectory;
        Ref<AssetManager> m_AssetManager;

        inline static Ref<Project> s_ActiveProject;
        friend class ProjectSerializer; 
    };
}