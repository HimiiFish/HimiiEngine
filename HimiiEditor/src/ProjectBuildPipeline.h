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

    struct ProjectBuildPipelineProgress
    {
        std::string StageLabel;
        float ProgressNormalized = 0.0f;
    };

    /// 编辑器侧完整发布管线：Export Template / 源码树回退、目录门禁、Release GameAssembly、
    /// 原子写出与 Runtime Release 布局一致的可运行目录（含 AssetRegistry.yaml）。
    /// 通过 BeginSession + UpdateSession 按帧推进，避免 UI 线程 busy-wait。
    class ProjectBuildPipeline
    {
    public:
        /// 校验路径与输出目录门禁，并启动 Release 脚本编译。失败时不进入会话。
        static ProjectBuildPipelineResult BeginSession(const std::filesystem::path& outputExecutablePath);

        /// 每帧推进；返回 true 表示仍在进行。结束后可用 GetSessionResult。
        static bool UpdateSession(ProjectBuildPipelineProgress& outProgress);

        static bool IsSessionActive();
        static ProjectBuildPipelineResult GetSessionResult();
        static std::filesystem::path GetSessionOutputDirectory();
    };
}
