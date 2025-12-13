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
    <OutputPath>bin\&(Configuration)</OutputPath>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>
  </PropertyGroup>

  <ItemGroup>
    <Reference Include="ScriptCore" Condition="Exists('$(HIMII_DIR)\bin\ScriptCore.dll')>
      <HintPath>$(HIMII_DIR)\bin\ScriptCore.dll</HintPath>
      <Private>false</Private>
    </Reference>
    <ProjectReference Include="..\..\..\..\ScriptCore\ScriptCore.csproj" Condition="!Exists('$(HIMII_DIR)\bin\ScriptCore.dll')>
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
    }

} // namespace Himii
