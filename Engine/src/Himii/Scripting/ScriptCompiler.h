#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace Himii
{
    class ScriptCompiler
    {
    public:
        using CompletionCallback = std::function<void(bool success)>;

        /// @param configuration Dotnet 配置名，默认 Debug（编辑器日常编译）；发布管线传 Release。
        /// @return false 表示未启动（已有编译进行中或正在关闭）。
        static bool RequestBuild(const std::filesystem::path& projectPath,
                                 CompletionCallback onComplete = nullptr,
                                 const std::string& configuration = "Debug");
        static void Update();

        static bool IsCompiling();
        static std::string GetLastLog();
        static int GetLastExitCode();

        /// 等待进行中的编译结束并终止 dotnet 子进程（编辑器退出时调用）。
        static void Shutdown();

    private:
        static void RunBuildThread(const std::filesystem::path projectPath, const std::string configuration);
    };
}
