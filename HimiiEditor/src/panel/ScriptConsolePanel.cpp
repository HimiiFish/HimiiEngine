#include "Hepch.h"
#include "ScriptConsolePanel.h"
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
            const std::string& line, std::filesystem::path& outPath, int& outLine)
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

    void ScriptConsolePanel::OnImGuiRender(bool* pOpen)
    {
        ImGui::SetNextWindowSize(ImVec2(520, 280), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Script Console", pOpen))
        {
            ImGui::End();
            return;
        }

        if (ScriptCompiler::IsCompiling())
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Compiling...");
        else if (ScriptCompiler::GetLastExitCode() == 0)
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Last build: Success");
        else if (ScriptCompiler::GetLastExitCode() >= 0)
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Last build: Failed (code %d)", ScriptCompiler::GetLastExitCode());

        ImGui::Separator();
        ImGui::TextDisabled("Click an error line to open in IDE (warnings are not clickable)");

        const std::string log = ScriptCompiler::GetLastLog();
        if (log.empty())
        {
            ImGui::TextDisabled("No build output yet.");
        }
        else
        {
            ImGui::BeginChild("ScriptLog", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

            std::istringstream stream(log);
            std::string line;
            int lineIndex = 0;
            while (std::getline(stream, line))
            {
                std::filesystem::path filePath;
                int fileLine = 0;
                const CompilerDiagnosticKind diagnosticKind =
                        TryParseCompilerLocation(line, filePath, fileLine);
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

            ImGui::EndChild();
        }

        ImGui::End();
    }
}
