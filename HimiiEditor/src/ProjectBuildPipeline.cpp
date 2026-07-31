#include "ProjectBuildPipeline.h"

#include "EngineCore/Core/Application.h"
#include "EngineCore/Core/Log.h"
#include "Project/Project.h"
#include "Project/ProjectSerializer.h"
#include "Module/Script/ScriptCompiler.h"

#include <cctype>
#include <vector>

namespace Himii
{
    namespace
    {
        enum class BuildSessionStage
        {
            Idle = 0,
            CompilingGameAssembly,
            ResolvingExportTemplate,
            WritingTemporaryPackage,
            CommittingPackage,
            Finished
        };

        struct BuildSessionState
        {
            bool Active = false;
            BuildSessionStage Stage = BuildSessionStage::Idle;
            ProjectBuildPipelineResult Result{};
            std::filesystem::path OutputExecutablePath;
            std::filesystem::path OutputDirectory;
            std::string ExecutableFileName;
            std::filesystem::path TemplateDirectory;
            std::filesystem::path TemplateRuntimeExecutable;
            std::filesystem::path TemplateEngineContentDirectory;
            std::vector<std::filesystem::path> NativeDependencyDllPaths;
            std::filesystem::path TemporaryDirectory;
            std::string StageLabel = "Starting...";
            float ProgressNormalized = 0.0f;
        };

        BuildSessionState& GetSession()
        {
            static BuildSessionState session;
            return session;
        }

        ProjectBuildPipelineResult MakeFailure(const std::string& errorMessage)
        {
            HIMII_CORE_ERROR("Build Pipeline failed: {0}", errorMessage);
            return {false, errorMessage};
        }

        ProjectBuildPipelineResult MakeSuccess()
        {
            return {true, {}};
        }

        void ResetSession()
        {
            BuildSessionState& session = GetSession();
            session = BuildSessionState{};
        }

        void FinishSession(const ProjectBuildPipelineResult& result)
        {
            BuildSessionState& session = GetSession();
            session.Result = result;
            session.Stage = BuildSessionStage::Finished;
            session.Active = false;
            session.ProgressNormalized = result.Succeeded ? 1.0f : session.ProgressNormalized;
            session.StageLabel = result.Succeeded ? "Build succeeded" : "Build failed";
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

        bool IsDirectoryEmpty(const std::filesystem::path& directoryPath, std::string& errorMessage)
        {
            std::error_code errorCode;
            auto directoryIterator = std::filesystem::directory_iterator(directoryPath, errorCode);
            if (errorCode)
            {
                errorMessage = "Failed to inspect output directory: " + errorCode.message();
                return false;
            }

            return directoryIterator == std::filesystem::directory_iterator{};
        }

        bool IsRecognizedPublishPackage(const std::filesystem::path& directoryPath)
        {
            return std::filesystem::exists(directoryPath / "Game.hproj")
                    && std::filesystem::exists(directoryPath / "HimiiEngine" / "engine.hpck")
                    && std::filesystem::exists(directoryPath / "GameAssembly.dll");
        }

        bool ValidateOutputDirectoryGate(const std::filesystem::path& outputDirectory,
                                         std::string& errorMessage)
        {
            if (!std::filesystem::exists(outputDirectory))
                return true;

            if (!std::filesystem::is_directory(outputDirectory))
            {
                errorMessage = "Output path parent is not a directory: " + outputDirectory.string();
                return false;
            }

            std::string directoryInspectError;
            const bool directoryIsEmpty = IsDirectoryEmpty(outputDirectory, directoryInspectError);
            if (!directoryInspectError.empty())
            {
                errorMessage = directoryInspectError;
                return false;
            }

            if (directoryIsEmpty)
                return true;

            if (IsRecognizedPublishPackage(outputDirectory))
                return true;

            errorMessage =
                    "Output directory is not empty and is not a previous Himii publish package. "
                    "Choose an empty folder, or a folder that already contains Game.hproj, "
                    "HimiiEngine/engine.hpck, and GameAssembly.dll.";
            return false;
        }

        bool IsExportTemplateDirectoryValid(const std::filesystem::path& templateDirectory)
        {
            return std::filesystem::exists(templateDirectory / "HimiiRuntime.exe")
                    && std::filesystem::exists(templateDirectory / "HimiiEngine" / "engine.hpck")
                    && std::filesystem::exists(templateDirectory / "HimiiEngine" / "ScriptCore.dll")
                    && std::filesystem::exists(templateDirectory / "HimiiEngine" /
                                               "ScriptCore.runtimeconfig.json");
        }

        bool ResolveExportTemplateDirectory(std::filesystem::path& outTemplateDirectory,
                                            std::string& errorMessage)
        {
            const std::filesystem::path editorExecutableDirectory =
                    Application::Get().GetExecutableDir();
            const std::filesystem::path embeddedTemplateDirectory =
                    editorExecutableDirectory / "ExportTemplates" / "Windows";
            const std::filesystem::path sourceTreeRuntimeDirectory =
                    editorExecutableDirectory.parent_path().parent_path() / "HimiiRuntime" / "Release";

            if (IsExportTemplateDirectoryValid(embeddedTemplateDirectory))
            {
                outTemplateDirectory = embeddedTemplateDirectory;
                HIMII_CORE_INFO("Build Pipeline: using Export Templates at {0}",
                                embeddedTemplateDirectory.string());
                return true;
            }

            if (IsExportTemplateDirectoryValid(sourceTreeRuntimeDirectory))
            {
                outTemplateDirectory = sourceTreeRuntimeDirectory;
                HIMII_CORE_INFO("Build Pipeline: using source-tree Release Runtime at {0}",
                                sourceTreeRuntimeDirectory.string());
                return true;
            }

            const bool embeddedTemplateDirectoryExists =
                    std::filesystem::exists(embeddedTemplateDirectory);
            if (!embeddedTemplateDirectoryExists)
            {
                errorMessage =
                        "Export Templates not found at: " + embeddedTemplateDirectory.string() +
                        ". Install or rebuild the editor with Release Export Templates "
                        "(cmake --install / Release Editor POST_BUILD). "
                        "Source-tree fallback also missing: " + sourceTreeRuntimeDirectory.string();
            }
            else
            {
                errorMessage =
                        "Export Templates at " + embeddedTemplateDirectory.string() +
                        " are incomplete (need HimiiRuntime.exe, HimiiEngine/engine.hpck, "
                        "ScriptCore.dll, ScriptCore.runtimeconfig.json). "
                        "Source-tree Release Runtime fallback also missing or incomplete: " +
                        sourceTreeRuntimeDirectory.string();
            }

            return false;
        }

        bool CollectNativeDependencyDllPaths(const std::filesystem::path& templateDirectory,
                                             std::vector<std::filesystem::path>& outDllPaths,
                                             std::string& errorMessage)
        {
            outDllPaths.clear();

            std::error_code errorCode;
            for (const auto& directoryEntry :
                 std::filesystem::directory_iterator(templateDirectory, errorCode))
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

                outDllPaths.push_back(directoryEntry.path());
            }

            if (errorCode)
            {
                errorMessage = "Failed to scan Export Template directory for DLLs: " +
                               errorCode.message();
                return false;
            }

            for (const auto& dependencyDllPath : outDllPaths)
            {
                if (!std::filesystem::exists(dependencyDllPath))
                {
                    errorMessage = "Dependency DLL missing: " + dependencyDllPath.string();
                    return false;
                }
            }

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

            const ProjectConfig originalConfiguration = Project::GetConfig();
            if (originalConfiguration.StartScene.is_absolute())
            {
                errorMessage =
                        "StartScene must be a path relative to assets. Absolute StartScene is not allowed: " +
                        originalConfiguration.StartScene.string();
                return false;
            }

            // 深拷贝后改写发布字段；序列化期间临时写回 active config，作用域结束立即还原，不触碰源 .hproj。
            struct ActiveConfigurationRestorer
            {
                ProjectConfig OriginalConfiguration;
                ~ActiveConfigurationRestorer() { Project::GetConfig() = OriginalConfiguration; }
            } configurationRestorer{originalConfiguration};

            Project::GetConfig().ScriptModulePath = "GameAssembly.dll";
            Project::GetConfig().AssetDirectory = "assets";

            ProjectSerializer serializer(Project::GetActive());
            const bool serializeSucceeded =
                    serializer.Serialize(destinationProjectFilePath, true /* omitScriptIDEFields */);

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
                std::error_code restoreErrorCode;
                if (std::filesystem::exists(backupDirectory))
                    std::filesystem::rename(backupDirectory, outputDirectory, restoreErrorCode);

                errorMessage = "Failed to commit build package: " + errorCode.message();
                return false;
            }

            RemoveDirectoryRecursiveIfExists(backupDirectory);
            return true;
        }

        bool ValidateProjectInputs(std::string& errorMessage)
        {
            if (!Project::GetActive())
            {
                errorMessage = "No active project. Open a project before building.";
                return false;
            }

            const ProjectConfig& projectConfiguration = Project::GetConfig();
            if (projectConfiguration.StartScene.is_absolute())
            {
                errorMessage =
                        "StartScene must be a path relative to assets. Absolute StartScene is not allowed: " +
                        projectConfiguration.StartScene.string();
                return false;
            }

            const std::filesystem::path projectDirectory = Project::GetProjectDirectory();
            const std::filesystem::path sourceProjectFile =
                    projectDirectory / (projectConfiguration.Name + ".hproj");
            const std::filesystem::path assetDirectory = Project::GetAssetDirectory();
            const std::filesystem::path startScenePath =
                    assetDirectory / projectConfiguration.StartScene;
            const std::filesystem::path csharpProjectPath = projectDirectory / "GameAssembly.csproj";

            if (!std::filesystem::exists(sourceProjectFile))
            {
                errorMessage = "Source project file missing: " + sourceProjectFile.string();
                return false;
            }

            if (!std::filesystem::exists(assetDirectory) || !std::filesystem::is_directory(assetDirectory))
            {
                errorMessage = "Project assets directory missing: " + assetDirectory.string();
                return false;
            }

            if (projectConfiguration.StartScene.empty() || !std::filesystem::exists(startScenePath))
            {
                errorMessage =
                        "StartScene file missing: " + startScenePath.string() +
                        " (check Project Settings → Start Scene).";
                return false;
            }

            if (!std::filesystem::exists(csharpProjectPath))
            {
                errorMessage = "GameAssembly.csproj not found: " + csharpProjectPath.string();
                return false;
            }

            const std::filesystem::path sourceAssetRegistryPath = Project::GetAssetRegistryPath();
            if (!std::filesystem::exists(sourceAssetRegistryPath))
            {
                errorMessage =
                        "AssetRegistry.yaml missing: " + sourceAssetRegistryPath.string() +
                        ". Save the project / run Build after assets are registered.";
                return false;
            }

            return true;
        }

        bool StartReleaseCompile(std::string& errorMessage)
        {
            const std::filesystem::path csharpProjectPath =
                    Project::GetProjectDirectory() / "GameAssembly.csproj";

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

            return true;
        }

        bool WriteTemporaryPackageContents(BuildSessionState& session, std::string& errorMessage)
        {
            const std::filesystem::path temporaryDirectory = session.TemporaryDirectory;
            const std::filesystem::path temporaryEngineContentDirectory =
                    temporaryDirectory / "HimiiEngine";
            const std::filesystem::path releaseGameAssemblyPath =
                    Project::GetProjectDirectory() / "bin" / "Release" / "GameAssembly.dll";
            const std::filesystem::path assetDirectory = Project::GetAssetDirectory();
            const std::filesystem::path sourceAssetRegistryPath = Project::GetAssetRegistryPath();

            if (!CopyFileRequired(session.TemplateRuntimeExecutable,
                                  temporaryDirectory / session.ExecutableFileName, errorMessage))
                return false;

            for (const auto& dependencyDllPath : session.NativeDependencyDllPaths)
            {
                if (!CopyFileRequired(dependencyDllPath,
                                      temporaryDirectory / dependencyDllPath.filename(), errorMessage))
                    return false;
            }

            if (!CopyFileRequired(session.TemplateEngineContentDirectory / "engine.hpck",
                                  temporaryEngineContentDirectory / "engine.hpck", errorMessage))
                return false;

            if (!CopyFileRequired(session.TemplateEngineContentDirectory / "ScriptCore.dll",
                                  temporaryEngineContentDirectory / "ScriptCore.dll", errorMessage))
                return false;

            if (!CopyFileRequired(session.TemplateEngineContentDirectory / "ScriptCore.runtimeconfig.json",
                                  temporaryEngineContentDirectory / "ScriptCore.runtimeconfig.json",
                                  errorMessage))
                return false;

            if (!CopyDirectoryRecursiveRequired(assetDirectory, temporaryDirectory / "assets",
                                                errorMessage))
                return false;

            if (!CopyFileRequired(sourceAssetRegistryPath, temporaryDirectory / "AssetRegistry.yaml",
                                  errorMessage))
                return false;

            if (!CopyFileRequired(releaseGameAssemblyPath, temporaryDirectory / "GameAssembly.dll",
                                  errorMessage))
                return false;

            if (!WritePublishProjectFile(temporaryDirectory / "Game.hproj", errorMessage))
                return false;

            return true;
        }
    }

    ProjectBuildPipelineResult ProjectBuildPipeline::BeginSession(
            const std::filesystem::path& outputExecutablePath)
    {
        if (GetSession().Active)
            return MakeFailure("A Build Pipeline session is already in progress.");

        ResetSession();

        std::string errorMessage;
        if (!ValidateProjectInputs(errorMessage))
            return MakeFailure(errorMessage);

        if (outputExecutablePath.empty())
            return MakeFailure("Output executable path is empty.");

        const std::filesystem::path outputDirectory = outputExecutablePath.parent_path();
        if (outputDirectory.empty())
            return MakeFailure("Output directory could not be resolved from: " +
                               outputExecutablePath.string());

        const std::string executableFileName = outputExecutablePath.filename().string();
        if (executableFileName.empty())
            return MakeFailure("Output executable file name is empty.");

        if (!ValidateOutputDirectoryGate(outputDirectory, errorMessage))
            return MakeFailure(errorMessage);

        if (!StartReleaseCompile(errorMessage))
            return MakeFailure(errorMessage);

        BuildSessionState& session = GetSession();
        session.Active = true;
        session.Stage = BuildSessionStage::CompilingGameAssembly;
        session.OutputExecutablePath = outputExecutablePath;
        session.OutputDirectory = outputDirectory;
        session.ExecutableFileName = executableFileName;
        session.StageLabel = "Compiling Release GameAssembly...";
        session.ProgressNormalized = 0.1f;
        session.Result = {};

        HIMII_CORE_INFO("Build Pipeline starting. Output: {0}", outputDirectory.string());
        return MakeSuccess();
    }

    bool ProjectBuildPipeline::UpdateSession(ProjectBuildPipelineProgress& outProgress)
    {
        BuildSessionState& session = GetSession();
        if (!session.Active)
        {
            outProgress.StageLabel = session.StageLabel;
            outProgress.ProgressNormalized = session.ProgressNormalized;
            return false;
        }

        outProgress.StageLabel = session.StageLabel;
        outProgress.ProgressNormalized = session.ProgressNormalized;

        std::string errorMessage;

        switch (session.Stage)
        {
            case BuildSessionStage::CompilingGameAssembly:
            {
                ScriptCompiler::Update();
                session.StageLabel = "Compiling Release GameAssembly...";
                session.ProgressNormalized = 0.35f;
                outProgress = {session.StageLabel, session.ProgressNormalized};

                if (ScriptCompiler::IsCompiling())
                    return true;

                if (ScriptCompiler::GetLastExitCode() != 0)
                {
                    errorMessage = "Release GameAssembly compile failed. See Script Console for details.";
                    const std::string compileLog = ScriptCompiler::GetLastLog();
                    if (!compileLog.empty())
                        HIMII_CORE_ERROR("Release compile log:\n{0}", compileLog);
                    FinishSession(MakeFailure(errorMessage));
                    outProgress = {session.StageLabel, session.ProgressNormalized};
                    return false;
                }

                const std::filesystem::path releaseGameAssemblyPath =
                        Project::GetProjectDirectory() / "bin" / "Release" / "GameAssembly.dll";
                if (!std::filesystem::exists(releaseGameAssemblyPath))
                {
                    FinishSession(MakeFailure(
                            "Release GameAssembly.dll not found after compile: " +
                            releaseGameAssemblyPath.string()));
                    outProgress = {session.StageLabel, session.ProgressNormalized};
                    return false;
                }

                HIMII_CORE_INFO("Build Pipeline: Release GameAssembly compile succeeded.");
                session.Stage = BuildSessionStage::ResolvingExportTemplate;
                session.StageLabel = "Resolving Export Templates...";
                session.ProgressNormalized = 0.55f;
                outProgress = {session.StageLabel, session.ProgressNormalized};
                return true;
            }

            case BuildSessionStage::ResolvingExportTemplate:
            {
                if (!ResolveExportTemplateDirectory(session.TemplateDirectory, errorMessage))
                {
                    FinishSession(MakeFailure(errorMessage));
                    outProgress = {session.StageLabel, session.ProgressNormalized};
                    return false;
                }

                session.TemplateRuntimeExecutable = session.TemplateDirectory / "HimiiRuntime.exe";
                session.TemplateEngineContentDirectory = session.TemplateDirectory / "HimiiEngine";

                if (!CollectNativeDependencyDllPaths(session.TemplateDirectory,
                                                     session.NativeDependencyDllPaths, errorMessage))
                {
                    FinishSession(MakeFailure(errorMessage));
                    outProgress = {session.StageLabel, session.ProgressNormalized};
                    return false;
                }

                session.TemporaryDirectory =
                        session.OutputDirectory.parent_path() /
                        (session.OutputDirectory.filename().string() + ".himii-build-tmp");
                const std::filesystem::path temporaryBackupDirectory =
                        session.OutputDirectory.parent_path() /
                        (session.OutputDirectory.filename().string() + ".himii-build-bak");

                RemoveDirectoryRecursiveIfExists(session.TemporaryDirectory);
                RemoveDirectoryRecursiveIfExists(temporaryBackupDirectory);

                {
                    std::error_code errorCode;
                    std::filesystem::create_directories(session.TemporaryDirectory, errorCode);
                    if (errorCode)
                    {
                        FinishSession(MakeFailure(
                                "Failed to create temporary build directory: " +
                                session.TemporaryDirectory.string() + " (" + errorCode.message() + ")"));
                        outProgress = {session.StageLabel, session.ProgressNormalized};
                        return false;
                    }
                }

                session.Stage = BuildSessionStage::WritingTemporaryPackage;
                session.StageLabel = "Writing package files...";
                session.ProgressNormalized = 0.7f;
                outProgress = {session.StageLabel, session.ProgressNormalized};
                return true;
            }

            case BuildSessionStage::WritingTemporaryPackage:
            {
                if (!WriteTemporaryPackageContents(session, errorMessage))
                {
                    RemoveDirectoryRecursiveIfExists(session.TemporaryDirectory);
                    FinishSession(MakeFailure(errorMessage));
                    outProgress = {session.StageLabel, session.ProgressNormalized};
                    return false;
                }

                session.Stage = BuildSessionStage::CommittingPackage;
                session.StageLabel = "Committing package...";
                session.ProgressNormalized = 0.9f;
                outProgress = {session.StageLabel, session.ProgressNormalized};
                return true;
            }

            case BuildSessionStage::CommittingPackage:
            {
                if (!CommitTemporaryPackage(session.TemporaryDirectory, session.OutputDirectory,
                                            errorMessage))
                {
                    RemoveDirectoryRecursiveIfExists(session.TemporaryDirectory);
                    FinishSession(MakeFailure(errorMessage));
                    outProgress = {session.StageLabel, session.ProgressNormalized};
                    return false;
                }

                HIMII_CORE_INFO("Build Pipeline succeeded. Package ready at: {0}",
                                session.OutputDirectory.string());
                FinishSession(MakeSuccess());
                outProgress = {session.StageLabel, session.ProgressNormalized};
                return false;
            }

            case BuildSessionStage::Idle:
            case BuildSessionStage::Finished:
            default:
                session.Active = false;
                outProgress = {session.StageLabel, session.ProgressNormalized};
                return false;
        }
    }

    bool ProjectBuildPipeline::IsSessionActive()
    {
        return GetSession().Active;
    }

    ProjectBuildPipelineResult ProjectBuildPipeline::GetSessionResult()
    {
        return GetSession().Result;
    }

    std::filesystem::path ProjectBuildPipeline::GetSessionOutputDirectory()
    {
        return GetSession().OutputDirectory;
    }
}
