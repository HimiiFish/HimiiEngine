#include "Hepch.h"
#include "ConsolePanel.h"
#include "EngineCore/Core/ConsoleLog.h"

#include <imgui.h>

namespace Himii
{
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

    void ConsolePanel::OnImGuiRender(bool* pOpen)
    {
        ImGui::SetNextWindowSize(ImVec2(520, 280), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Console", pOpen))
        {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Clear"))
            ConsoleLog::Clear();

        ImGui::SameLine();
        ImGui::Checkbox("Show Engine Logs", &m_ShowEngineLogs);
        ImGui::SameLine();
        ImGui::Checkbox("Auto Scroll", &m_AutoScroll);

        ImGui::Separator();

        ImGui::BeginChild("ConsoleLog", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        const std::vector<ConsoleLogEntry> entries = ConsoleLog::GetEntries();

        for (const auto& entry : entries)
        {
            if (!m_ShowEngineLogs && entry.Source != "Script")
                continue;

            ImVec4 color = GetLogColor(entry.Level);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(entry.Message.c_str());
            ImGui::PopStyleColor();
        }

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
}
