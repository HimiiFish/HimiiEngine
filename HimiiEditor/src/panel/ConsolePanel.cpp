#include "Hepch.h"
#include "ConsolePanel.h"
#include "EngineCore/Core/ConsoleLog.h"
#include "Module/Script/ScriptCompiler.h"
#include "Module/Script/ScriptIDELauncher.h"
#include "Project/Project.h"

#include <imgui.h>
#include <sstream>

namespace Himii
{
    enum class CompilerDiagnosticKind
    {
        None = 0,
        Error,
        Warning
    };

    static CompilerDiagnosticKind TryParseCompilerLocation(
            const std::string &line, std::filesystem::path &outPath, int &outLine)
    {
        const size_t errorPosition = line.find(": error");
        const size_t warningPosition = line.find(": warning");

        size_t messagePosition = std::string::npos;
        CompilerDiagnosticKind kind = CompilerDiagnosticKind::None;
        if (errorPosition != std::string::npos
            && (warningPosition == std::string::npos || errorPosition < warningPosition))
        {
            messagePosition = errorPosition;
            kind = CompilerDiagnosticKind::Error;
        }
        else if (warningPosition != std::string::npos)
        {
            messagePosition = warningPosition;
            kind = CompilerDiagnosticKind::Warning;
        }

        if (messagePosition == std::string::npos)
            return CompilerDiagnosticKind::None;

        const size_t closeParenthesis = line.rfind(')', messagePosition);
        if (closeParenthesis == std::string::npos)
            return CompilerDiagnosticKind::None;

        const size_t openParenthesis = line.rfind('(', closeParenthesis);
        if (openParenthesis == std::string::npos || openParenthesis >= closeParenthesis)
            return CompilerDiagnosticKind::None;

        std::string lineNumberText = line.substr(openParenthesis + 1, closeParenthesis - openParenthesis - 1);
        const size_t commaPosition = lineNumberText.find(',');
        if (commaPosition != std::string::npos)
            lineNumberText = lineNumberText.substr(0, commaPosition);

        if (lineNumberText.empty())
            return CompilerDiagnosticKind::None;

        try
        {
            outLine = std::stoi(lineNumberText);
        }
        catch (...)
        {
            return CompilerDiagnosticKind::None;
        }

        outPath = line.substr(0, openParenthesis);
        std::string pathText = outPath.string();
        while (!pathText.empty() && (pathText.front() == ' ' || pathText.front() == '\t'))
            pathText.erase(pathText.begin());

        if (pathText.empty())
            return CompilerDiagnosticKind::None;

        outPath = pathText;
        return kind;
    }

    static ImVec4 GetLogColor(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Warning:
            case LogLevel::Core_Warning:
                return ImVec4(1.0f, 0.85f, 0.2f, 1.0f);
            case LogLevel::Error:
            case LogLevel::Core_Error:
                return ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
            case LogLevel::Trace:
            case LogLevel::Core_Trace:
                return ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
            default:
                return ImVec4(0.92f, 0.92f, 0.92f, 1.0f);
        }
    }

    static bool IsEngineVerboseLevel(LogLevel level)
    {
        return level == LogLevel::Info || level == LogLevel::Core_Info || level == LogLevel::Trace
               || level == LogLevel::Core_Trace;
    }

    static void DrawCompileLines()
    {
        const std::string log = ScriptCompiler::GetLastLog();
        if (log.empty())
        {
            ImGui::TextDisabled("No compile output yet.");
            return;
        }

        std::istringstream stream(log);
        std::string line;
        int lineIndex = 0;
        while (std::getline(stream, line))
        {
            std::filesystem::path filePath;
            int fileLine = 0;
            const CompilerDiagnosticKind diagnosticKind = TryParseCompilerLocation(line, filePath, fileLine);
            const bool isError = diagnosticKind == CompilerDiagnosticKind::Error;
            const bool isWarning = diagnosticKind == CompilerDiagnosticKind::Warning;

            if (isError)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
            else if (isWarning)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.25f, 1.0f));

            ImGui::PushID(lineIndex++);
            if (isError)
            {
                if (ImGui::Selectable(line.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
                {
                    if (Project::GetActive())
                    {
                        std::filesystem::path absolutePath = filePath;
                        if (!absolutePath.is_absolute())
                            absolutePath = Project::GetProjectDirectory() / filePath;

                        ScriptIDELauncher::OpenScript(
                                Project::GetProjectDirectory(),
                                Project::GetConfig().Name,
                                absolutePath,
                                fileLine);
                    }
                }
            }
            else
            {
                ImGui::TextUnformatted(line.c_str());
            }
            ImGui::PopID();

            if (isError || isWarning)
                ImGui::PopStyleColor();
        }
    }

    void ConsolePanel::OnImGuiRender(bool *open)
    {
        ImGui::SetNextWindowSize(ImVec2(520, 280), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Console", open))
        {
            ImGui::End();
            return;
        }

        if (ScriptCompiler::IsCompiling())
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Compiling...");
        else if (ScriptCompiler::GetLastExitCode() == 0)
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Last build: Success");
        else if (ScriptCompiler::GetLastExitCode() >= 0)
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Last build: Failed (code %d)",
                    ScriptCompiler::GetLastExitCode());
        else
            ImGui::TextDisabled("Last build: —");

        if (ImGui::Button("Clear"))
            ConsoleLog::Clear();

        ImGui::SameLine();
        ImGui::Checkbox("Script", &m_ShowScript);
        ImGui::SameLine();
        ImGui::Checkbox("Compile", &m_ShowCompile);
        ImGui::SameLine();
        ImGui::Checkbox("Engine", &m_ShowEngine);
        ImGui::SameLine();
        ImGui::BeginDisabled(!m_ShowEngine);
        ImGui::Checkbox("Engine Info / Trace", &m_ShowEngineVerbose);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::Checkbox("Auto Scroll", &m_AutoScroll);

        ImGui::Separator();

        ImGui::BeginChild("ConsoleLog", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (m_ShowCompile)
        {
            ImGui::TextDisabled("Compile");
            ImGui::TextDisabled("Click an error line to open in IDE (warnings are not clickable)");
            DrawCompileLines();
            if (m_ShowScript || m_ShowEngine)
                ImGui::Separator();
        }

        if (m_ShowScript || m_ShowEngine)
        {
            if (m_ShowCompile)
                ImGui::TextDisabled("Runtime");

            const std::vector<ConsoleLogEntry> entries = ConsoleLog::GetEntries();
            for (const auto &entry : entries)
            {
                const bool isScript = entry.Source == "Script";
                if (isScript && !m_ShowScript)
                    continue;
                if (!isScript && !m_ShowEngine)
                    continue;
                if (!isScript && !m_ShowEngineVerbose && IsEngineVerboseLevel(entry.Level))
                    continue;

                ImVec4 color = GetLogColor(entry.Level);
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                if (isScript)
                {
                    ImGui::TextUnformatted(entry.Message.c_str());
                }
                else
                {
                    const std::string displayText = "[Engine] " + entry.Message;
                    ImGui::TextUnformatted(displayText.c_str());
                }
                ImGui::PopStyleColor();
            }
        }

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
}
