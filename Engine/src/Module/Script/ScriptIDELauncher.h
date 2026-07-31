#pragma once

#include "Module/Script/ScriptIDE.h"
#include <filesystem>

namespace Himii
{
    class Project;

    class ScriptIDELauncher
    {
    public:
        static ScriptIDEConfig ResolveConfig();
        static bool OpenProject(const std::filesystem::path& projectDir, const std::string& projectName);
        static bool OpenScript(const std::filesystem::path& projectDir, const std::string& projectName,
                               const std::filesystem::path& scriptFile, int line = 0);

    private:
        static std::filesystem::path GetSolutionPath(const std::filesystem::path& projectDir, const std::string& projectName);
        static std::string ExpandArgumentTemplate(const std::string& templateStr,
                                                  const std::filesystem::path& projectDir,
                                                  const std::string& projectName,
                                                  const std::filesystem::path& solutionPath,
                                                  const std::filesystem::path& filePath,
                                                  int line = 0);
        static bool LaunchProcess(const std::string& executable, const std::string& arguments);
        static bool LaunchVisualStudio(const std::filesystem::path& solutionPath, const std::filesystem::path& filePath,
                                       int line);
        static bool LaunchVSCode(const std::filesystem::path& projectDir, const std::filesystem::path& filePath,
                                 int line);
        static bool LaunchRider(const std::filesystem::path& solutionPath, const std::filesystem::path& filePath,
                                int line);
        static bool LaunchCustom(const ScriptIDEConfig& config,
                                 const std::filesystem::path& projectDir,
                                 const std::string& projectName,
                                 const std::filesystem::path& solutionPath,
                                 const std::filesystem::path& filePath,
                                 int line = 0);
        static std::filesystem::path FindDevenvExecutable();
        static std::filesystem::path FindVSCodeExecutable();
        static std::filesystem::path FindRiderExecutable();
    };
}
