#include "Hepch.h"
#include "ScriptCompiler.h"

#include <atomic>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <stdlib.h>
#else
#include <stdlib.h>
#endif

namespace Himii
{
    namespace
    {
        std::mutex s_LogMutex;
        std::string s_LastLog;
        std::atomic<bool> s_IsCompiling{false};
        std::atomic<int> s_LastExitCode{-1};
        std::atomic<bool> s_HasPendingResult{false};
        std::atomic<bool> s_PendingSuccess{false};
        std::atomic<bool> s_ShutdownRequested{false};
        ScriptCompiler::CompletionCallback s_PendingCallback;
        std::mutex s_CallbackMutex;
        std::thread s_BuildThread;

#ifdef _WIN32
        HANDLE s_JobObject = nullptr;
        HANDLE s_ActiveProcessHandle = nullptr;

        HANDLE GetOrCreateBuildJobObject()
        {
            if (s_JobObject)
                return s_JobObject;

            s_JobObject = CreateJobObjectW(nullptr, nullptr);
            if (!s_JobObject)
                return nullptr;

            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobLimits{};
            jobLimits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            if (!SetInformationJobObject(
                    s_JobObject, JobObjectExtendedLimitInformation, &jobLimits, sizeof(jobLimits)))
            {
                CloseHandle(s_JobObject);
                s_JobObject = nullptr;
                return nullptr;
            }

            return s_JobObject;
        }

        bool AssignProcessToBuildJob(HANDLE processHandle)
        {
            HANDLE jobObject = GetOrCreateBuildJobObject();
            if (!jobObject)
                return false;

            return AssignProcessToJobObject(jobObject, processHandle) != FALSE;
        }

        static void DisableDotnetBuildServerForChildProcesses()
        {
#ifdef _WIN32
            _wputenv_s(L"DOTNET_CLI_USE_MSBUILD_SERVER", L"0");
            _wputenv_s(L"MSBUILDDISABLENODEREUSE", L"1");
#else
            setenv("DOTNET_CLI_USE_MSBUILD_SERVER", "0", 1);
            setenv("MSBUILDDISABLENODEREUSE", "1", 1);
#endif
        }

        int RunDotnetBuildWindows(const std::filesystem::path& projectPath,
                                  const std::string& configuration,
                                  std::string& output)
        {
            DisableDotnetBuildServerForChildProcesses();

            SECURITY_ATTRIBUTES securityAttributes{};
            securityAttributes.nLength = sizeof(securityAttributes);
            securityAttributes.bInheritHandle = TRUE;

            HANDLE standardOutputReadHandle = nullptr;
            HANDLE standardOutputWriteHandle = nullptr;
            if (!CreatePipe(&standardOutputReadHandle, &standardOutputWriteHandle, &securityAttributes, 0))
            {
                output = "[ScriptCompiler] Failed to create output pipe.\n";
                return -1;
            }

            SetHandleInformation(standardOutputReadHandle, HANDLE_FLAG_INHERIT, 0);

            std::wstring configurationWide(configuration.begin(), configuration.end());
            std::wstring commandLine = L"dotnet build \"";
            commandLine += projectPath.wstring();
            commandLine += L"\" -c ";
            commandLine += configurationWide;
            commandLine += L" -v minimal";

            std::vector<wchar_t> commandBuffer(commandLine.begin(), commandLine.end());
            commandBuffer.push_back(L'\0');

            STARTUPINFOW startupInfo{};
            startupInfo.cb = sizeof(startupInfo);
            startupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
            startupInfo.hStdOutput = standardOutputWriteHandle;
            startupInfo.hStdError = standardOutputWriteHandle;
            startupInfo.wShowWindow = SW_HIDE;

            PROCESS_INFORMATION processInformation{};

            if (!CreateProcessW(
                    nullptr,
                    commandBuffer.data(),
                    nullptr,
                    nullptr,
                    TRUE,
                    CREATE_NO_WINDOW,
                    nullptr,
                    nullptr,
                    &startupInfo,
                    &processInformation))
            {
                CloseHandle(standardOutputReadHandle);
                CloseHandle(standardOutputWriteHandle);
                output = "[ScriptCompiler] Failed to start dotnet build process.\n";
                return -1;
            }

            CloseHandle(standardOutputWriteHandle);
            s_ActiveProcessHandle = processInformation.hProcess;
            AssignProcessToBuildJob(processInformation.hProcess);

            char readBuffer[512];
            DWORD bytesRead = 0;
            while (ReadFile(standardOutputReadHandle, readBuffer, sizeof(readBuffer) - 1, &bytesRead, nullptr)
                   && bytesRead > 0)
            {
                readBuffer[bytesRead] = '\0';
                output += readBuffer;
            }

            WaitForSingleObject(processInformation.hProcess, INFINITE);

            DWORD exitCode = static_cast<DWORD>(-1);
            GetExitCodeProcess(processInformation.hProcess, &exitCode);

            CloseHandle(processInformation.hThread);
            CloseHandle(processInformation.hProcess);
            CloseHandle(standardOutputReadHandle);

            s_ActiveProcessHandle = nullptr;
            return static_cast<int>(exitCode);
        }
#endif
    }

    bool ScriptCompiler::RequestBuild(const std::filesystem::path& projectPath,
                                      CompletionCallback onComplete,
                                      const std::string& configuration)
    {
        if (s_IsCompiling.load() || s_ShutdownRequested.load())
            return false;

        const std::string resolvedConfiguration = configuration.empty() ? "Debug" : configuration;

        if (s_BuildThread.joinable())
            s_BuildThread.join();

        {
            std::lock_guard<std::mutex> lock(s_CallbackMutex);
            s_PendingCallback = std::move(onComplete);
        }

        s_IsCompiling.store(true);
        s_BuildThread = std::thread(RunBuildThread, projectPath, resolvedConfiguration);
        return true;
    }

    void ScriptCompiler::RunBuildThread(const std::filesystem::path projectPath,
                                        const std::string configuration)
    {
        std::string output;
        int exitCode = -1;

        if (s_ShutdownRequested.load())
        {
            output = "[ScriptCompiler] Build cancelled during shutdown.\n";
            exitCode = -1;
        }
#ifdef _WIN32
        else
        {
            exitCode = RunDotnetBuildWindows(projectPath, configuration, output);
        }
#else
        else
        {
            DisableDotnetBuildServerForChildProcesses();

            std::ostringstream command;
            command << "dotnet build \"" << projectPath.string()
                    << "\" -c " << configuration << " -v minimal 2>&1";

            FILE* pipe = popen(command.str().c_str(), "r");
            if (pipe)
            {
                char buffer[512];
                while (fgets(buffer, sizeof(buffer), pipe))
                    output += buffer;
                exitCode = pclose(pipe);
            }
            else
            {
                output = "[ScriptCompiler] Failed to start dotnet build process.\n";
            }
        }
#endif

        {
            std::lock_guard<std::mutex> lock(s_LogMutex);
            s_LastLog = std::move(output);
            s_LastExitCode.store(exitCode);
        }

        s_PendingSuccess.store(exitCode == 0);
        s_HasPendingResult.store(true);
        s_IsCompiling.store(false);
    }

    void ScriptCompiler::Update()
    {
        if (!s_HasPendingResult.exchange(false))
            return;

        CompletionCallback callback;
        {
            std::lock_guard<std::mutex> lock(s_CallbackMutex);
            callback = std::move(s_PendingCallback);
            s_PendingCallback = nullptr;
        }

        if (callback)
            callback(s_PendingSuccess.load());
    }

    void ScriptCompiler::Shutdown()
    {
        s_ShutdownRequested.store(true);

#ifdef _WIN32
        if (s_ActiveProcessHandle)
        {
            TerminateProcess(s_ActiveProcessHandle, 1);
            WaitForSingleObject(s_ActiveProcessHandle, 5000);
        }

        if (s_JobObject)
        {
            CloseHandle(s_JobObject);
            s_JobObject = nullptr;
        }
#endif

        if (s_BuildThread.joinable())
            s_BuildThread.join();
    }

    bool ScriptCompiler::IsCompiling()
    {
        return s_IsCompiling.load();
    }

    std::string ScriptCompiler::GetLastLog()
    {
        std::lock_guard<std::mutex> lock(s_LogMutex);
        return s_LastLog;
    }

    int ScriptCompiler::GetLastExitCode()
    {
        return s_LastExitCode.load();
    }
}
