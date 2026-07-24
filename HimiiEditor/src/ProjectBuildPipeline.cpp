#include "ProjectBuildPipeline.h"

#include "Himii/Core/Application.h"
#include "Himii/Core/Log.h"
#include "Himii/Project/Project.h"
#include "Himii/Project/ProjectSerializer.h"
#include "Himii/Scripting/ScriptCompiler.h"

#include <cctype>
#include <chrono>
#include <thread>
#include <vector>

namespace Himii
{
    namespace
    {
        ProjectBuildPipelineResult MakeFailure(const std::string& errorMessage)
        {
            HIMII_CORE_ERROR("Build Pipeline failed: {0}", errorMessage);
            return {false, errorMessage};
        }

        ProjectBuildPipelineResult MakeSuccess()
        {
            return {true, {}};
        }

        void RemoveDirectoryRecursiveIfExists(const std::filesystem::path& directoryPath)
        {
            if (!std::filesystem::exists(directoryPath))
                return;

            std::error_code errorCode;
            std::filesystem::remove_all(directoryPath, errorCode);
            if (errorCode)
            {
                HIMII_CORE_WARNING("Failed to remove directory {0}: {1}",
                                   directoryPath.string(), errorCode.message());
            }
        }

        bool CopyFileRequired(const std::filesystem::path& sourcePath,
                              const std::filesystem::path& destinationPath,
                              std::string& errorMessage)
        {
            if (!std::filesystem::exists(sourcePath))
            {
                errorMessage = "Required file missing: " + sourcePath.string();
                return false;
            }

            std::error_code errorCode;
            std::filesystem::create_directories(destinationPath.parent_path(), errorCode);
            if (errorCode)
            {
                errorMessage = "Failed to create directory for " + destinationPath.string() + ": " +
                               errorCode.message();
                return false;
            }

            errorCode.clear();
            std::filesystem::copy_file(sourcePath, destinationPath,
                                       std::filesystem::copy_options::overwrite_existing, errorCode);
            if (errorCode)
            {
                errorMessage = "Failed to copy " + sourcePath.string() + " -> " +
                               destinationPath.string() + ": " + errorCode.message();
                return false;
            }

            return true;
        }

        bool CopyDirectoryRecursiveRequired(const std::filesystem::path& sourceDirectory,
                                            const std::filesystem::path& destinationDirectory,
                                            std::string& errorMessage)
        {
            if (!std::filesystem::exists(sourceDirectory) ||
                !std::filesystem::is_directory(sourceDirectory))
            {
                errorMessage = "Required directory missing: " + sourceDirectory.string();
                return false;
            }

            std::error_code errorCode;
            std::filesystem::create_directories(destinationDirectory, errorCode);
            if (errorCode)
            {
                errorMessage = "Failed to create directory " + destinationDirectory.string() + ": " +
                               errorCode.message();
                return false;
            }

            errorCode.clear();
            std::filesystem::copy(sourceDirectory, destinationDirectory,
                                  std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing,
                                  errorCode);
            if (errorCode)
            {
                errorMessage = "Failed to copy directory " + sourceDirectory.string() + " -> " +
                               destinationDirectory.string() + ": " + errorCode.message();
                return false;
            }

            return true;
        }

        bool WaitForScriptCompileCompletion(std::string& errorMessage)
        {
            while (ScriptCompiler::IsCompiling())
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

            // 触发完成回调（若有），并读取退出码。
            ScriptCompiler::Update();

            if (ScriptCompiler::GetLastExitCode() != 0)
            {
                errorMessage = "Release GameAssembly compile failed. See Script Console for details.";
                const std::string compileLog = ScriptCompiler::GetLastLog();
                if (!compileLog.empty())
                    HIMII_CORE_ERROR("Release compile log:\n{0}", compileLog);
                return false;
            }

            return true;
        }

        bool CompileReleaseGameAssembly(const std::filesystem::path& csharpProjectPath,
                                        std::string& errorMessage)
        {
            if (!std::filesystem::exists(csharpProjectPath))
            {
                errorMessage = "GameAssembly.csproj not found: " + csharpProjectPath.string();
                return false;
            }

            if (ScriptCompiler::IsCompiling())
            {
                errorMessage = "Cannot start Build Pipeline while a script compile is already in progress.";
                return false;
            }

            HIMII_CORE_INFO("Build Pipeline: compiling GameAssembly (Release)...");
            if (!ScriptCompiler::RequestBuild(csharpProjectPath, nullptr, "Release"))
            {
                errorMessage = "Failed to start Release GameAssembly compile.";
                return false;
            }

            if (!WaitForScriptCompileCompletion(errorMessage))
                return false;

            HIMII_CORE_INFO("Build Pipeline: Release GameAssembly compile succeeded.");
            return true;
        }

        bool WritePublishProjectFile(const std::filesystem::path& destinationProjectFilePath,
                                     std::string& errorMessage)
        {
            if (!Project::GetActive())
            {
                errorMessage = "No active project.";
                return false;
            }

            // 深拷贝当前配置后仅改写发布字段；序列化期间临时写回 active config，作用域结束立即还原，不触碰源 .hproj。
            const ProjectConfig originalConfiguration = Project::GetConfig();
            struct ActiveConfigurationRestorer
            {
                const ProjectConfig& OriginalConfiguration;
                ~ActiveConfigurationRestorer() { Project::GetConfig() = OriginalConfiguration; }
            } configurationRestorer{originalConfiguration};

            Project::GetConfig().ScriptModulePath = "GameAssembly.dll";

            ProjectSerializer serializer(Project::GetActive());
            const bool serializeSucceeded = serializer.Serialize(destinationProjectFilePath);

            if (!serializeSucceeded || !std::filesystem::exists(destinationProjectFilePath))
            {
                errorMessage = "Failed to write publish Game.hproj: " + destinationProjectFilePath.string();
                return false;
            }

            return true;
        }

        bool CommitTemporaryPackage(const std::filesystem::path& temporaryDirectory,
                                    const std::filesystem::path& outputDirectory,
                                    std::string& errorMessage)
        {
            const std::filesystem::path backupDirectory =
                    outputDirectory.parent_path() /
                    (outputDirectory.filename().string() + ".himii-build-bak");

            RemoveDirectoryRecursiveIfExists(backupDirectory);

            std::error_code errorCode;
            if (std::filesystem::exists(outputDirectory))
            {
                std::filesystem::rename(outputDirectory, backupDirectory, errorCode);
                if (errorCode)
                {
                    errorMessage =
                            "Failed to move existing package aside (is the game exe still running?): " +
                            errorCode.message();
                    return false;
                }
            }

            errorCode.clear();
            std::filesystem::rename(temporaryDirectory, outputDirectory, errorCode);
            if (errorCode)
            {
                // 尝试把旧包移回。
                std::error_code restoreErrorCode;
                if (std::filesystem::exists(backupDirectory))
                    std::filesystem::rename(backupDirectory, outputDirectory, restoreErrorCode);

                errorMessage = "Failed to commit build package: " + errorCode.message();
                return false;
            }

            RemoveDirectoryRecursiveIfExists(backupDirectory);
            return true;
        }
    }

    ProjectBuildPipelineResult ProjectBuildPipeline::Build(
            const std::filesystem::path& outputExecutablePath)
    {
        if (!Project::GetActive())
            return MakeFailure("No active project. Open a project before building.");

        if (outputExecutablePath.empty())
            return MakeFailure("Output executable path is empty.");

        const std::filesystem::path outputDirectory = outputExecutablePath.parent_path();
        if (outputDirectory.empty())
            return MakeFailure("Output directory could not be resolved from: " +
                               outputExecutablePath.string());

        const std::string executableFileName = outputExecutablePath.filename().string();
        if (executableFileName.empty())
            return MakeFailure("Output executable file name is empty.");

        HIMII_CORE_INFO("Build Pipeline starting. Output: {0}", outputDirectory.string());

        // --- Resolve Release Runtime sources (always Release, independent of Editor config) ---
        const std::filesystem::path editorExecutableDirectory =
                Application::Get().GetExecutableDir();
        const std::filesystem::path binaryRoot =
                editorExecutableDirectory.parent_path().parent_path();
        const std::filesystem::path releaseRuntimeDirectory =
                binaryRoot / "HimiiRuntime" / "Release";
        const std::filesystem::path releaseRuntimeExecutable =
                releaseRuntimeDirectory / "HimiiRuntime.exe";
        const std::filesystem::path releaseEngineContentDirectory =
                releaseRuntimeDirectory / "HimiiEngine";
        const std::filesystem::path releaseEnginePackFile =
                releaseEngineContentDirectory / "engine.hpck";
        const std::filesystem::path releaseScriptCoreAssembly =
                releaseEngineContentDirectory / "ScriptCore.dll";
        const std::filesystem::path releaseScriptCoreRuntimeConfig =
                releaseEngineContentDirectory / "ScriptCore.runtimeconfig.json";

        if (!std::filesystem::exists(releaseRuntimeExecutable))
        {
            return MakeFailure(
                    "Release HimiiRuntime.exe not found at: " + releaseRuntimeExecutable.string() +
                    ". Build HimiiRuntime (Release) first (cmake --build with Release config).");
        }

        if (!std::filesystem::exists(releaseEnginePackFile))
        {
            return MakeFailure(
                    "engine.hpck not found at: " + releaseEnginePackFile.string() +
                    ". Build Release HimiiRuntime (PackEngineResources) first.");
        }

        if (!std::filesystem::exists(releaseScriptCoreAssembly) ||
            !std::filesystem::exists(releaseScriptCoreRuntimeConfig))
        {
            return MakeFailure(
                    "ScriptCore pair missing under Release HimiiEngine/. Expected: " +
                    releaseScriptCoreAssembly.string() + " and " +
                    releaseScriptCoreRuntimeConfig.string());
        }

        // --- Project-side required inputs ---
        const std::filesystem::path projectDirectory = Project::GetProjectDirectory();
        const ProjectConfig& projectConfiguration = Project::GetConfig();
        const std::filesystem::path sourceProjectFile =
                projectDirectory / (projectConfiguration.Name + ".hproj");
        const std::filesystem::path assetDirectory = Project::GetAssetDirectory();
        const std::filesystem::path startScenePath =
                assetDirectory / projectConfiguration.StartScene;
        const std::filesystem::path csharpProjectPath = projectDirectory / "GameAssembly.csproj";
        const std::filesystem::path releaseGameAssemblyPath =
                projectDirectory / "bin" / "Release" / "GameAssembly.dll";

        if (!std::filesystem::exists(sourceProjectFile))
            return MakeFailure("Source project file missing: " + sourceProjectFile.string());

        if (!std::filesystem::exists(assetDirectory) || !std::filesystem::is_directory(assetDirectory))
            return MakeFailure("Project assets directory missing: " + assetDirectory.string());

        if (projectConfiguration.StartScene.empty() || !std::filesystem::exists(startScenePath))
        {
            return MakeFailure(
                    "StartScene file missing: " + startScenePath.string() +
                    " (check Project Settings → Start Scene).");
        }

        // Collect native dependency DLLs from Release Runtime output (exclude GameAssembly).
        std::vector<std::filesystem::path> nativeDependencyDllPaths;
        {
            std::error_code errorCode;
            for (const auto& directoryEntry :
                 std::filesystem::directory_iterator(releaseRuntimeDirectory, errorCode))
            {
                if (errorCode)
                    break;

                if (!directoryEntry.is_regular_file())
                    continue;

                std::string extension = directoryEntry.path().extension().string();
                for (char& character : extension)
                    character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

                if (extension != ".dll")
                    continue;

                std::string stemName = directoryEntry.path().stem().string();
                for (char& character : stemName)
                    character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

                if (stemName == "gameassembly")
                    continue;

                nativeDependencyDllPaths.push_back(directoryEntry.path());
            }

            if (errorCode)
            {
                return MakeFailure("Failed to scan Release Runtime directory for DLLs: " +
                                   errorCode.message());
            }
        }

        for (const auto& dependencyDllPath : nativeDependencyDllPaths)
        {
            if (!std::filesystem::exists(dependencyDllPath))
            {
                return MakeFailure("Dependency DLL missing: " + dependencyDllPath.string());
            }
        }

        // --- Compile Release GameAssembly ---
        std::string stageErrorMessage;
        if (!CompileReleaseGameAssembly(csharpProjectPath, stageErrorMessage))
            return MakeFailure(stageErrorMessage);

        if (!std::filesystem::exists(releaseGameAssemblyPath))
        {
            return MakeFailure(
                    "Release GameAssembly.dll not found after compile: " +
                    releaseGameAssemblyPath.string());
        }

        // --- Stage into temporary directory ---
        const std::filesystem::path temporaryDirectory =
                outputDirectory.parent_path() /
                (outputDirectory.filename().string() + ".himii-build-tmp");
        const std::filesystem::path temporaryBackupDirectory =
                outputDirectory.parent_path() /
                (outputDirectory.filename().string() + ".himii-build-bak");

        RemoveDirectoryRecursiveIfExists(temporaryDirectory);
        RemoveDirectoryRecursiveIfExists(temporaryBackupDirectory);

        {
            std::error_code errorCode;
            std::filesystem::create_directories(temporaryDirectory, errorCode);
            if (errorCode)
            {
                return MakeFailure("Failed to create temporary build directory: " +
                                   temporaryDirectory.string() + " (" + errorCode.message() + ")");
            }
        }

        const auto failAndCleanupTemporary = [&](const std::string& message) -> ProjectBuildPipelineResult {
            RemoveDirectoryRecursiveIfExists(temporaryDirectory);
            return MakeFailure(message);
        };

        // Runtime exe renamed to user-specified name
        if (!CopyFileRequired(releaseRuntimeExecutable, temporaryDirectory / executableFileName,
                              stageErrorMessage))
            return failAndCleanupTemporary(stageErrorMessage);

        // Native dependency DLLs beside exe
        for (const auto& dependencyDllPath : nativeDependencyDllPaths)
        {
            if (!CopyFileRequired(dependencyDllPath,
                                  temporaryDirectory / dependencyDllPath.filename(),
                                  stageErrorMessage))
                return failAndCleanupTemporary(stageErrorMessage);
        }

        // HimiiEngine/ engine.hpck + ScriptCore pair (no loose engine assets)
        const std::filesystem::path temporaryEngineContentDirectory =
                temporaryDirectory / "HimiiEngine";
        if (!CopyFileRequired(releaseEnginePackFile,
                              temporaryEngineContentDirectory / "engine.hpck",
                              stageErrorMessage))
            return failAndCleanupTemporary(stageErrorMessage);

        if (!CopyFileRequired(releaseScriptCoreAssembly,
                              temporaryEngineContentDirectory / "ScriptCore.dll",
                              stageErrorMessage))
            return failAndCleanupTemporary(stageErrorMessage);

        if (!CopyFileRequired(releaseScriptCoreRuntimeConfig,
                              temporaryEngineContentDirectory / "ScriptCore.runtimeconfig.json",
                              stageErrorMessage))
            return failAndCleanupTemporary(stageErrorMessage);

        // Project assets/
        if (!CopyDirectoryRecursiveRequired(assetDirectory, temporaryDirectory / "assets",
                                            stageErrorMessage))
            return failAndCleanupTemporary(stageErrorMessage);

        // GameAssembly.dll at package root
        if (!CopyFileRequired(releaseGameAssemblyPath, temporaryDirectory / "GameAssembly.dll",
                              stageErrorMessage))
            return failAndCleanupTemporary(stageErrorMessage);

        // Publish Game.hproj (does not mutate source .hproj on disk)
        if (!WritePublishProjectFile(temporaryDirectory / "Game.hproj", stageErrorMessage))
            return failAndCleanupTemporary(stageErrorMessage);

        // --- Atomic commit ---
        if (!CommitTemporaryPackage(temporaryDirectory, outputDirectory, stageErrorMessage))
        {
            RemoveDirectoryRecursiveIfExists(temporaryDirectory);
            return MakeFailure(stageErrorMessage);
        }

        HIMII_CORE_INFO("Build Pipeline succeeded. Package ready at: {0}", outputDirectory.string());
        return MakeSuccess();
    }
}
