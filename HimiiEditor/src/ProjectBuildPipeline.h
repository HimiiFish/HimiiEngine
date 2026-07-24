#pragma once

#include <filesystem>
#include <string>

namespace Himii
{
    struct ProjectBuildPipelineResult
    {
        bool Succeeded = false;
        std::string ErrorMessage;
    };

    /// 编辑器侧完整发布管线：校验 Release Runtime / engine.hpck、编译 Release GameAssembly、
    /// 原子写出与 HimiiRuntime Release 布局一致的可运行目录。
    class ProjectBuildPipeline
    {
    public:
        /// @param outputExecutablePath 用户选定的输出 exe 完整路径（如 MyGame/MyGame.exe）。
        static ProjectBuildPipelineResult Build(const std::filesystem::path& outputExecutablePath);
    };
}
