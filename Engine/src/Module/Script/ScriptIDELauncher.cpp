#include "Hepch.h"
#include "ScriptIDELauncher.h"
#include "EngineCore/Editor/EditorSettings.h"
#include "Project/Project.h"

#include <sstream>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Himii
{
    ScriptIDEConfig ScriptIDELauncher::ResolveConfig()
    {
        ScriptIDEConfig config = EditorSettings::Get().DefaultScriptIDE;

        if (!Project::GetActive())
            return config;

        const auto& projectConfig = Project::GetConfig();
        if (projectConfig.ScriptIDEOverride != ScriptIDEType::Inherit)
        {
            config.Type = projectConfig.ScriptIDEOverride;
            if (config.Type == ScriptIDEType::Custom)
            {
                if (!projectConfig.CustomScriptIDEPath.empty())
                    config.CustomExecutable = projectConfig.CustomScriptIDEPath;
                if (!projectConfig.CustomScriptIDEArguments.empty())
                    config.CustomArguments = projectConfig.CustomScriptIDEArguments;
            }
        }

        return config;
    }

    std::filesystem::path ScriptIDELauncher::GetSolutionPath(const std::filesystem::path& projectDir,
                                                             const std::string& projectName)
    {
        return projectDir / (projectName + ".sln");
    }

    std::string ScriptIDELauncher::ExpandArgumentTemplate(const std::string& templateStr,
                                                          const std::filesystem::path& projectDir,
                                                          const std::string& projectName,
                                                          const std::filesystem::path& solutionPath,
                                                          const std::filesystem::path& filePath,
                                                          int line)
    {
        auto replaceAll = [](std::string str, const std::string& from, const std::string& to)
        {
            size_t pos = 0;
            while ((pos = str.find(from, pos)) != std::string::npos)
            {
                str.replace(pos, from.length(), to);
                pos += to.length();
            }
            return str;
        };

        std::string result = templateStr;
        result = replaceAll(result, "{ProjectDir}", "\"" + projectDir.string() + "\"");
        result = replaceAll(result, "{ProjectName}", projectName);
        result = replaceAll(result, "{Solution}", "\"" + solutionPath.string() + "\"");
        if (!filePath.empty())
            result = replaceAll(result, "{File}", "\"" + filePath.string() + "\"");
        else
            result = replaceAll(result, "{File}", "");
        result = replaceAll(result, "{Line}", line > 0 ? std::to_string(line) : "0");
        return result;
    }

    bool ScriptIDELauncher::LaunchProcess(const std::string& executable, const std::string& arguments)
    {
        if (!std::filesystem::exists(executable))
        {
            HIMII_CORE_ERROR("Executable not found: {0}", executable);
            return false;
        }

#ifdef _WIN32
        std::string commandLine = "\"" + executable + "\"";
        if (!arguments.empty())
            commandLine += " " + arguments;

        std::vector<char> buffer(commandLine.begin(), commandLine.end());
        buffer.push_back('\0');

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        BOOL success = CreateProcessA(
            nullptr,
            buffer.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi);

        if (!success)
        {
            HIMII_CORE_ERROR("Failed to launch process: {0}", commandLine);
            return false;
        }

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        HIMII_CORE_INFO("Launched: {0}", commandLine);
        return true;
#else
        std::string cmd = "\"" + executable + "\"";
        if (!arguments.empty())
            cmd += " " + arguments;
        int result = std::system(cmd.c_str());
        return result == 0;
#endif
    }

    std::filesystem::path ScriptIDELauncher::FindDevenvExecutable()
    {
#ifdef _WIN32
        const char* programFilesX86 = std::getenv("ProgramFiles(x86)");
        if (programFilesX86)
        {
            std::filesystem::path vswhere = std::filesystem::path(programFilesX86) /
                                            "Microsoft Visual Studio/Installer/vswhere.exe";
            if (std::filesystem::exists(vswhere))
            {
                std::string cmd = "\"" + vswhere.string() +
                                  "\" -latest -products * -requires Microsoft.Component.MSBuild "
                                  "-find **/devenv.exe";
                FILE* pipe = _popen(cmd.c_str(), "r");
                if (pipe)
                {
                    char buffer[512];
                    if (fgets(buffer, sizeof(buffer), pipe))
                    {
                        std::string path = buffer;
                        while (!path.empty() && (path.back() == '\n' || path.back() == '\r'))
                            path.pop_back();
                        _pclose(pipe);
                        if (!path.empty() && std::filesystem::exists(path))
                            return path;
                    }
                    _pclose(pipe);
                }
            }
        }

        const std::vector<std::filesystem::path> fallbackPaths = {
            "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/devenv.exe",
            "C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE/devenv.exe",
            "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/devenv.exe",
            "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/devenv.exe",
        };

        for (const auto& path : fallbackPaths)
        {
            if (std::filesystem::exists(path))
                return path;
        }
#endif
        return {};
    }

    std::filesystem::path ScriptIDELauncher::FindVSCodeExecutable()
    {
#ifdef _WIN32
        FILE* pipe = _popen("where code 2>nul", "r");
        if (pipe)
        {
            char buffer[512];
            if (fgets(buffer, sizeof(buffer), pipe))
            {
                std::string path = buffer;
                while (!path.empty() && (path.back() == '\n' || path.back() == '\r'))
                    path.pop_back();
                _pclose(pipe);
                if (!path.empty() && std::filesystem::exists(path))
                    return path;
            }
            _pclose(pipe);
        }

        const char* localAppData = std::getenv("LOCALAPPDATA");
        if (localAppData)
        {
            std::filesystem::path codeExe = std::filesystem::path(localAppData) /
                                            "Programs/Microsoft VS Code/Code.exe";
            if (std::filesystem::exists(codeExe))
                return codeExe;
        }
#endif
        return {};
    }

    std::filesystem::path ScriptIDELauncher::FindRiderExecutable()
    {
#ifdef _WIN32
        FILE* pipe = _popen("where rider64 2>nul", "r");
        if (pipe)
        {
            char buffer[512];
            if (fgets(buffer, sizeof(buffer), pipe))
            {
                std::string path = buffer;
                while (!path.empty() && (path.back() == '\n' || path.back() == '\r'))
                    path.pop_back();
                _pclose(pipe);
                if (!path.empty() && std::filesystem::exists(path))
                    return path;
            }
            _pclose(pipe);
        }

        const char* localAppData = std::getenv("LOCALAPPDATA");
        if (localAppData)
        {
            std::filesystem::path toolbox = std::filesystem::path(localAppData) / "JetBrains/Toolbox/apps";
            if (std::filesystem::exists(toolbox))
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(toolbox))
                {
                    if (entry.path().filename() == "rider64.exe")
                        return entry.path();
                }
            }
        }

        const std::vector<std::filesystem::path> fallbackPaths = {
            "C:/Program Files/JetBrains/JetBrains Rider 2024.3/bin/rider64.exe",
            "C:/Program Files/JetBrains/JetBrains Rider 2024.2/bin/rider64.exe",
        };
        for (const auto& path : fallbackPaths)
        {
            if (std::filesystem::exists(path))
                return path;
        }
#endif
        return {};
    }

    bool ScriptIDELauncher::LaunchVisualStudio(const std::filesystem::path& solutionPath,
                                               const std::filesystem::path& filePath, int line)
    {
        auto devenv = FindDevenvExecutable();
        if (devenv.empty())
        {
            HIMII_CORE_ERROR("Visual Studio (devenv.exe) not found. Install VS or use a Custom IDE path.");
            return false;
        }

        // 打开脚本也必须有 .sln：禁止 /edit（会走简化布局，且易误路由到其它已开实例）
        if (!std::filesystem::exists(solutionPath))
        {
            HIMII_CORE_ERROR("Solution file not found: {0}", solutionPath.string());
            return false;
        }

        // 与 Open C# Solution 相同：先传解决方案路径（CreateProcess 用常规双引号即可）
        std::string args = "\"" + solutionPath.string() + "\"";

        if (!filePath.empty())
        {
            if (!std::filesystem::exists(filePath))
            {
                HIMII_CORE_ERROR("Script file not found: {0}", filePath.string());
                return false;
            }

            std::error_code absoluteErrorCode;
            std::filesystem::path absoluteFilePath = std::filesystem::absolute(filePath, absoluteErrorCode);
            if (absoluteErrorCode)
                absoluteFilePath = filePath;

            // 作为第二位置参数打开文件，避免 /command + 嵌套引号在 CreateProcess 下被拆成「幽灵文件路径」
            // （此前使用 File.OpenFile 与 \" 转义时，VS 会弹「命令行中指定了以下文件…未能找到」）
            args += " \"" + absoluteFilePath.string() + "\"";

            if (line > 0)
                args += " /command \"Edit.GoTo " + std::to_string(line) + "\"";
        }

        return LaunchProcess(devenv.string(), args);
    }

    bool ScriptIDELauncher::LaunchVSCode(const std::filesystem::path& projectDir,
                                         const std::filesystem::path& filePath, int line)
    {
        auto codeExe = FindVSCodeExecutable();
        if (codeExe.empty())
        {
            HIMII_CORE_ERROR("VS Code (code) not found. Install VS Code or add it to PATH.");
            return false;
        }

        std::string args = "\"" + projectDir.string() + "\"";
        if (!filePath.empty() && std::filesystem::exists(filePath))
        {
            if (line > 0)
                args += " -g \"" + filePath.string() + ":" + std::to_string(line) + "\"";
            else
                args += " -g \"" + filePath.string() + "\"";
        }

        return LaunchProcess(codeExe.string(), args);
    }

    bool ScriptIDELauncher::LaunchRider(const std::filesystem::path& solutionPath,
                                        const std::filesystem::path& filePath, int line)
    {
        auto riderExe = FindRiderExecutable();
        if (riderExe.empty())
        {
            HIMII_CORE_ERROR("JetBrains Rider (rider64.exe) not found.");
            return false;
        }

        if (!std::filesystem::exists(solutionPath) && filePath.empty())
        {
            HIMII_CORE_ERROR("Solution file not found: {0}", solutionPath.string());
            return false;
        }

        std::string args;
        if (std::filesystem::exists(solutionPath))
            args = "\"" + solutionPath.string() + "\"";

        if (!filePath.empty() && std::filesystem::exists(filePath))
        {
            if (!args.empty())
                args += " ";
            if (line > 0)
                args += "--line " + std::to_string(line) + " ";
            args += "\"" + filePath.string() + "\"";
        }

        if (args.empty())
        {
            HIMII_CORE_ERROR("Nothing to open for Rider.");
            return false;
        }

        return LaunchProcess(riderExe.string(), args);
    }

    bool ScriptIDELauncher::LaunchCustom(const ScriptIDEConfig& config,
                                         const std::filesystem::path& projectDir,
                                         const std::string& projectName,
                                         const std::filesystem::path& solutionPath,
                                         const std::filesystem::path& filePath,
                                         int line)
    {
        if (config.CustomExecutable.empty())
        {
            HIMII_CORE_ERROR("Custom IDE executable path is empty.");
            return false;
        }

        std::string args = ExpandArgumentTemplate(
            config.CustomArguments, projectDir, projectName, solutionPath, filePath, line);
        return LaunchProcess(config.CustomExecutable, args);
    }

    bool ScriptIDELauncher::OpenProject(const std::filesystem::path& projectDir, const std::string& projectName)
    {
        auto config = ResolveConfig();
        auto solutionPath = GetSolutionPath(projectDir, projectName);

        switch (config.Type)
        {
            case ScriptIDEType::VisualStudio:
                return LaunchVisualStudio(solutionPath, {}, 0);
            case ScriptIDEType::VSCode:
                return LaunchVSCode(projectDir, {}, 0);
            case ScriptIDEType::Rider:
                return LaunchRider(solutionPath, {}, 0);
            case ScriptIDEType::Custom:
                return LaunchCustom(config, projectDir, projectName, solutionPath, {}, 0);
            default:
                HIMII_CORE_ERROR("Invalid script IDE configuration.");
                return false;
        }
    }

    bool ScriptIDELauncher::OpenScript(const std::filesystem::path& projectDir, const std::string& projectName,
                                       const std::filesystem::path& scriptFile, int line)
    {
        if (!std::filesystem::exists(scriptFile))
        {
            HIMII_CORE_ERROR("Script file not found: {0}", scriptFile.string());
            return false;
        }

        auto config = ResolveConfig();
        auto solutionPath = GetSolutionPath(projectDir, projectName);

        switch (config.Type)
        {
            case ScriptIDEType::VisualStudio:
                return LaunchVisualStudio(solutionPath, scriptFile, line);
            case ScriptIDEType::VSCode:
                return LaunchVSCode(projectDir, scriptFile, line);
            case ScriptIDEType::Rider:
                return LaunchRider(solutionPath, scriptFile, line);
            case ScriptIDEType::Custom:
                return LaunchCustom(config, projectDir, projectName, solutionPath, scriptFile, line);
            default:
                HIMII_CORE_ERROR("Invalid script IDE configuration.");
                return false;
        }
    }
}
