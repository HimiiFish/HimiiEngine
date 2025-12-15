#include "Project.h"
#include "hepch.h"

#include "ProjectSerializer.h"

namespace Himii
{

    Ref<Project> Project::New()
    {
        s_ActiveProject = CreateRef<Project>();
        return s_ActiveProject;
    }

    Ref<Project> Project::Load(const std::filesystem::path &path)
    {
        Ref<Project> project = CreateRef<Project>();

        ProjectSerializer serializer(project);
        if (serializer.Deserialize(path))
        {
            project->m_ProjectDirectory = path.parent_path();
            s_ActiveProject = project;
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

        // 2. 自动生成 GameAssembly.csproj
        // 我们把 XML 内容直接写死在代码里（作为模板）
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
    <Reference Include="ScriptCore" Condition="Exists('$(HIMII_DIR)\ScriptCore.dll')>
      <HintPath>$(HIMII_DIR)\ScriptCore.dll</HintPath>
      <Private>false</Private>
    </Reference>
    <ProjectReference Include="..\..\..\ScriptCore\ScriptCore.dll" Condition="!Exists('$(HIMII_DIR)\ScriptCore.dll')>
      <Private>false</Private>
    </ProjectReference>
  </ItemGroup>

  <ItemGroup>
    <Compile Include="assets\scripts\**\*.cs" />
  </ItemGroup>
</Project>
)";

        std::ofstream csprojFile(projectDir / "GameAssembly.csproj");
        csprojFile << ss.str();
        csprojFile.close();

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

        // 2. 定义 ScriptCore 项目 (方便查看引擎源码)
        // 这里的路径需要回退 4 层找到 ScriptCore
        std::string scriptCorePath =  "ScriptCore.csproj";
        std::string scriptCoreGUID = "{45962852-2567-41C2-B358-132717009044}"; // 随便编一个不同的
        ss1 << "Project(\"{9A19103F-16F7-4668-BE54-9A1E7A4F7556}\") = \"ScriptCore\", \"" << scriptCorePath << "\", \""
           << scriptCoreGUID << "\"\n";
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
        // ScriptCore
        ss1 << "\t\t" << scriptCoreGUID << ".Debug|Any CPU.ActiveCfg = Debug|Any CPU\n";
        ss1 << "\t\t" << scriptCoreGUID << ".Debug|Any CPU.Build.0 = Debug|Any CPU\n";
        ss1 << "\t\t" << scriptCoreGUID << ".Release|Any CPU.ActiveCfg = Release|Any CPU\n";
        ss1 << "\t\t" << scriptCoreGUID << ".Release|Any CPU.Build.0 = Release|Any CPU\n";
        ss1 << "\tEndGlobalSection\n";
        ss1 << "EndGlobal\n";

        // 写入 .sln 文件 (文件名通常和项目名一致，例如 Sandbox.sln)
        std::ofstream slnFile(projectDir / (name + ".sln"));
        if (slnFile.is_open())
        {
            slnFile << ss1.str();
            slnFile.close();
        }
    }

} // namespace Himii
