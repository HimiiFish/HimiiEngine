#include "FontDiagnosticsPanel.h"
#include "imgui.h"
#include "EngineCore/Core/JobSystem.h"
#include <algorithm>

namespace Himii
{
    void FontDiagnosticsPanel::OnImGuiRender()
    {
        if (!m_Show)
            return;

        ImGui::Begin("Font Diagnostics", &m_Show);
        if (!m_Font)
        {
            ImGui::TextUnformatted("No font selected. Using default font if available.");
            m_Font = Font::GetDefault();
        }

        if (!m_Font)
        {
            ImGui::TextUnformatted("Default font is not initialized.");
            ImGui::End();
            return;
        }

        const FontDiagnosticsSnapshot snapshot = m_Font->GetDiagnosticsSnapshot();
        ImGui::Text("Path: %s", m_Font->GetFilePath().string().c_str());
        ImGui::Text("Face Index: %d", m_Font->GetFaceIndex());
        ImGui::Text("Glyph Count: %u", snapshot.GlyphCount);
        ImGui::Text("Page Count: %u", snapshot.PageCount);
        ImGui::Text("Pending Generations: %u", snapshot.PendingGenerationCount);
        ImGui::Text("Worker Tasks: %u", JobSystem::GetPendingWorkerTaskCount());
        ImGui::Text("Main Completions: %u", JobSystem::GetPendingMainThreadCompletionCount());
        ImGui::Text("Approx VRAM: %.2f MiB",
                    static_cast<double>(snapshot.ApproximateVideoMemoryBytes) / (1024.0 * 1024.0));
        if (!snapshot.LastError.empty())
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Last Error: %s", snapshot.LastError.c_str());

        ImGui::Separator();
        ImGui::Text("Missing Code Points: %zu", snapshot.MissingCodePoints.size());
        if (ImGui::BeginListBox("##MissingCodePoints", ImVec2(-1, 100)))
        {
            for (char32_t codePoint : snapshot.MissingCodePoints)
                ImGui::Text("U+%04X", static_cast<unsigned>(codePoint));
            ImGui::EndListBox();
        }

        ImGui::Separator();
        if (!snapshot.AtlasPages.empty())
        {
            m_SelectedPageIndex = std::clamp(
                    m_SelectedPageIndex, 0, static_cast<int>(snapshot.AtlasPages.size()) - 1);
            ImGui::SliderInt("Atlas Page", &m_SelectedPageIndex, 0,
                             static_cast<int>(snapshot.AtlasPages.size()) - 1);
            Ref<Texture2D> page = snapshot.AtlasPages[static_cast<size_t>(m_SelectedPageIndex)];
            if (page)
            {
                const float previewSize = 256.0f;
                ImGui::Image((ImTextureID)(intptr_t)page->GetRendererID(),
                             ImVec2(previewSize, previewSize), ImVec2(0, 1), ImVec2(1, 0));
            }
        }

        if (ImGui::Button("Preload ASCII + Common CJK Sample"))
        {
            m_Font->PreloadCharacters(
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
                    "文字渲染测试");
        }
        ImGui::SameLine();
        if (ImGui::Button("Preload Async Sample"))
        {
            m_Font->PreloadTextAsync("异步预加载 Asynchronous Preload 你好");
        }

        ImGui::End();
    }
}
