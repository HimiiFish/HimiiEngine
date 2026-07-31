#include "Project.h"
#include "Hepch.h"
#include "EngineCore/Core/Application.h"
#include "EngineCore/Core/FileSystem.h"
#include "Module/Render/Font.h"
#include "Resource/ResourceSystem.h"

#include "ProjectSerializer.h"

namespace Himii
{
    namespace
    {
        std::filesystem::path ResolveScriptCoreAssemblyPath()
        {
            // Debug: GetEngineDir() == exe dir (ScriptCore next to editor).
            // Release: GetEngineDir() == exe/HimiiEngine (published layout).
            return Application::GetEngineDir() / "ScriptCore.dll";
        }

        void CopyIfExists(const std::filesystem::path &sourcePath, const std::filesystem::path &destinationPath)
        {
            if (!std::filesystem::exists(sourcePath))
                return;

            std::error_code errorCode;
            std::filesystem::copy_file(sourcePath, destinationPath,
                                       std::filesystem::copy_options::overwrite_existing, errorCode);
            if (errorCode)
                HIMII_CORE_WARNING("Failed to copy {0}: {1}", sourcePath.string(), errorCode.message());
        }

        void RemoveLegacyScriptingApiStubFromProject(const std::filesystem::path &projectDir)
        {
            const std::filesystem::path scriptingApiStubDirectory = projectDir / "assets" / "scripts" / "Himii";
            const std::filesystem::path serializeFieldStubPath = scriptingApiStubDirectory / "SerializeField.cs";

            std::error_code errorCode;
            if (std::filesystem::exists(serializeFieldStubPath))
            {
                std::filesystem::remove(serializeFieldStubPath, errorCode);
                if (errorCode)
                {
                    HIMII_CORE_WARNING("Failed to remove legacy stub {0}: {1}",
                                       serializeFieldStubPath.string(), errorCode.message());
                }
            }

            if (!std::filesystem::exists(scriptingApiStubDirectory))
                return;

            const bool directoryIsEmpty =
                std::filesystem::is_empty(scriptingApiStubDirectory, errorCode);
            if (!errorCode && directoryIsEmpty)
                std::filesystem::remove(scriptingApiStubDirectory, errorCode);
        }
    }

    void Project::SyncScriptCoreToProjectDirectory(const std::filesystem::path &projectDir)
    {
        const std::filesystem::path scriptCoreAssemblyPath = ResolveScriptCoreAssemblyPath();
        if (!std::filesystem::exists(scriptCoreAssemblyPath))
        {
            HIMII_CORE_WARNING("ScriptCore.dll not found, IDE may not resolve Himii API types.");
            return;
        }

        CopyIfExists(scriptCoreAssemblyPath, projectDir / "ScriptCore.dll");

        std::filesystem::path programDatabasePath = scriptCoreAssemblyPath;
        programDatabasePath.replace_extension(".pdb");
        CopyIfExists(programDatabasePath, projectDir / "ScriptCore.pdb");

        std::filesystem::path documentationPath = scriptCoreAssemblyPath;
        documentationPath.replace_extension(".xml");
        CopyIfExists(documentationPath, projectDir / "ScriptCore.xml");

        RemoveLegacyScriptingApiStubFromProject(projectDir);
    }

    Ref<Project> Project::New()
    {
        s_ActiveProject = CreateRef<Project>();
        s_ActiveProject->m_AssetManager = CreateRef<AssetManager>();
        ResourceSystem::Bind(s_ActiveProject->m_AssetManager);
        ResourceSystem::DeserializeAssetRegistry();
        return s_ActiveProject;
    }

    Ref<Project> Project::Load(const std::filesystem::path &path)
    {
        Ref<Project> project = CreateRef<Project>();
        project->m_AssetManager = CreateRef<AssetManager>();
        ProjectSerializer serializer(project);
        if (serializer.Deserialize(path))
        {
            project->m_ProjectDirectory = path.parent_path();
            s_ActiveProject = project;
            ResourceSystem::Bind(s_ActiveProject->m_AssetManager);
            ResourceSystem::DeserializeAssetRegistry();
            return s_ActiveProject;
        }

        return nullptr;
    }

    bool Project::SaveActive(const std::filesystem::path &path)
    {
        ProjectSerializer serializer(s_ActiveProject);
        if (serializer.Serialize(path))
        {
            s_ActiveProject->m_ProjectDirectory = path.parent_path();
            ResourceSystem::SerializeAssetRegistry();
            return true;
        }

        return false;
    }

    void Project::CreateProjectFiles(const std::string &name, const std::filesystem::path &projectDir)
    {
        if (!std::filesystem::exists(projectDir))
            std::filesystem::create_directories(projectDir);

        // 1. 创建标准目录结构
        std::filesystem::create_directories(projectDir / "assets" / "scenes");
        std::filesystem::create_directories(projectDir / "assets" / "scripts");
        std::filesystem::create_directories(projectDir / "assets" / "textures");
        std::filesystem::create_directories(projectDir / "assets" / "fonts");
        std::filesystem::create_directories(projectDir / "assets" / "skybox");

        SyncScriptCoreToProjectDirectory(projectDir);

        // 2. 自动生成 GameAssembly.csproj（引用项目根目录下的 ScriptCore.dll，便于 IDE 解析）
        std::stringstream ss;
        ss << R"(<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net8.0</TargetFramework>
    <ImplicitUsings>enable</ImplicitUsings>
    <Nullable>enable</Nullable>
    <OutputPath>bin\$(Configuration)</OutputPath>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>
  </PropertyGroup>

  <ItemGroup>
    <Using Include="HimiiEngine" />
  </ItemGroup>

  <ItemGroup>
    <Compile Include="assets\scripts\**\*.cs" />
  </ItemGroup>

  <ItemGroup>
    <Reference Include="ScriptCore">
      <HintPath>ScriptCore.dll</HintPath>
      <Private>false</Private>
    </Reference>
  </ItemGroup>
</Project>
)";

        std::ofstream csprojFile(projectDir / "GameAssembly.csproj");
        csprojFile << ss.str();
        csprojFile.close();

        const std::filesystem::path defaultScriptPath = projectDir / "assets" / "scripts" / "SampleScript.cs";
        if (!std::filesystem::exists(defaultScriptPath))
        {
            std::ofstream scriptFile(defaultScriptPath);
            if (scriptFile.is_open())
            {
                scriptFile << "using HimiiEngine;\n\n";
                scriptFile << "public class SampleScript : Entity\n";
                scriptFile << "{\n";
                scriptFile << "    [SerializeField]\n";
                scriptFile << "    private float moveSpeed = 5.0f;\n\n";
                scriptFile << "    public override void OnUpdate(float timestep)\n";
                scriptFile << "    {\n";
                scriptFile << "        // var delta = Time.DeltaTime;\n";
                scriptFile << "    }\n";
                scriptFile << "}\n";
            }
        }

        // 3. (可选) 生成一个空的默认场景，防止 StartScene 报错
        // 这一步比较复杂，需要 SceneSerializer 支持保存空场景，暂时跳过

        HIMII_CORE_INFO("Created new project structure at {0}", projectDir.string());

        std::stringstream ss1;

        // SLN 文件头
        ss1 << "Microsoft Visual Studio Solution File, Format Version 12.00\n";
        ss1 << "# Visual Studio Version 17\n";
        ss1 << "VisualStudioVersion = 17.0.31903.59\n";
        ss1 << "MinimumVisualStudioVersion = 10.0.40219.1\n";

        // 1. 定义 GameAssembly 项目
        // GUID 可以是随机生成的，这里为了演示暂时硬编码 (但在实际引擎中最好动态生成)
        std::string gameAssemblyGUID = "{52962852-2567-41C2-B358-132717009043}";
        ss1 << "Project(\"{9A19103F-16F7-4668-BE54-9A1E7A4F7556}\") = \"GameAssembly\", \"GameAssembly.csproj\", \""
           << gameAssemblyGUID << "\"\n";
        ss1 << "EndProject\n";

        // 3. 定义全局配置 (Debug/Release)
        ss1 << "Global\n";
        ss1 << "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n";
        ss1 << "\t\tDebug|Any CPU = Debug|Any CPU\n";
        ss1 << "\t\tRelease|Any CPU = Release|Any CPU\n";
        ss1 << "\tEndGlobalSection\n";

        // 4. 关联项目配置
        ss1 << "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n";
        // GameAssembly
        ss1 << "\t\t" << gameAssemblyGUID << ".Debug|Any CPU.ActiveCfg = Debug|Any CPU\n";
        ss1 << "\t\t" << gameAssemblyGUID << ".Debug|Any CPU.Build.0 = Debug|Any CPU\n";
        ss1 << "\t\t" << gameAssemblyGUID << ".Release|Any CPU.ActiveCfg = Release|Any CPU\n";
        ss1 << "\t\t" << gameAssemblyGUID << ".Release|Any CPU.Build.0 = Release|Any CPU\n";
        
        ss1 << "EndGlobal\n";

        // 写入 .sln 文件 (文件名通常和项目名一致，例如 Sandbox.sln)
        std::ofstream slnFile(projectDir / (name + ".sln"));
        if (slnFile.is_open())
        {
            slnFile << ss1.str();
            slnFile.close();
        }
    }

    std::filesystem::path Project::GetDefaultGameplayFontRelativePath()
    {
        return std::filesystem::path("fonts") / "msyh.ttc";
    }

    void Project::EnsureSeededDefaultAssets()
    {
        if (!ResourceSystem::IsBound())
            return;

        bool registryChanged = false;
        Ref<AssetManager> assetManager = ResourceSystem::GetAssetManager();

        auto copyEngineContentIntoProjectIfMissing =
                [&](const std::string &engineRelativePath, const std::filesystem::path &projectRelativePath) -> bool {
            const std::filesystem::path destinationPath = GetAssetFileSystemPath(projectRelativePath);
            bool didCopy = false;

            if (!std::filesystem::exists(destinationPath))
            {
                const std::filesystem::path sourcePath = FileSystem::MaterializeLooseFile(engineRelativePath);
                if (!std::filesystem::exists(sourcePath))
                {
                    HIMII_CORE_WARNING("Seed asset source missing: {0}", engineRelativePath);
                    return false;
                }

                std::error_code createDirectoryError;
                std::filesystem::create_directories(destinationPath.parent_path(), createDirectoryError);
                std::error_code copyError;
                std::filesystem::copy_file(sourcePath, destinationPath,
                                           std::filesystem::copy_options::overwrite_existing, copyError);
                if (copyError)
                {
                    HIMII_CORE_WARNING("Failed to seed project asset {0}: {1}", destinationPath.string(),
                                       copyError.message());
                    return false;
                }
                didCopy = true;
            }

            bool alreadyRegistered = false;
            for (const auto &[handle, metadata] : assetManager->GetAssetRegistry())
            {
                if (metadata.FilePath.generic_string() == projectRelativePath.generic_string())
                {
                    alreadyRegistered = true;
                    break;
                }
            }

            if (!alreadyRegistered)
            {
                const AssetHandle importedHandle = ResourceSystem::ImportAsset(projectRelativePath);
                if (importedHandle == 0)
                {
                    HIMII_CORE_WARNING("Seeded file but failed to register asset: {0}",
                                       projectRelativePath.string());
                    return didCopy;
                }
                registryChanged = true;
            }

            return true;
        };

        copyEngineContentIntoProjectIfMissing("assets/fonts/msyh.ttc", GetDefaultGameplayFontRelativePath());

        if (!GetConfig().Is2D)
        {
            static const char *skyboxFaceNames[] = {"right", "left", "top", "bottom", "front", "back"};
            for (const char *faceName : skyboxFaceNames)
            {
                const std::string engineRelativePath =
                        std::string("resources/skybox/") + faceName + ".bmp";
                const std::filesystem::path projectRelativePath =
                        std::filesystem::path("skybox") / (std::string(faceName) + ".bmp");
                copyEngineContentIntoProjectIfMissing(engineRelativePath, projectRelativePath);
            }
        }

        if (registryChanged)
            ResourceSystem::SerializeAssetRegistry();
    }

    void Project::InitializeGameplayDefaultFont()
    {
        if (!s_ActiveProject)
            return;

        const std::filesystem::path fontPath = GetAssetFileSystemPath(GetDefaultGameplayFontRelativePath());
        if (!std::filesystem::exists(fontPath))
        {
            HIMII_CORE_ERROR(
                    "Gameplay default font missing at {0}. Seed assets or place fonts/msyh.ttc under project assets.",
                    fontPath.string());
            return;
        }

        Font::InitDefault(fontPath);
    }

} // namespace Himii
