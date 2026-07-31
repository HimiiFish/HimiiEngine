#include "ParticleEmitterEditorPanel.h"

#include "InspectorControls.h"

#include "imgui.h"
#include <algorithm>
#include <cfloat>
#include <filesystem>
#include "Resource/AssetManager.h"
#include "Resource/AssetSerializer.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "Module/Render/Renderer2D.h"
#include "Module/Render/RenderCommand.h"
#include "Module/Render/Texture.h"
#include "EngineCore/Core/Log.h"
#include "Module/Particle/ParticleSystem.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Himii
{
    ParticleEmitterEditorPanel::ParticleEmitterEditorPanel()
        : m_Camera(-5.0f, 5.0f, -5.0f, 5.0f)
    {
        FramebufferSpecification fbSpec{ 320, 240 };
        fbSpec.Attachments = { FramebufferFormat::RGBA8, FramebufferFormat::Depth };
        m_Framebuffer = Framebuffer::Create(fbSpec);
    }

    void ParticleEmitterEditorPanel::Open(AssetHandle emitterHandle)
    {
        m_EmitterHandle = emitterHandle;
        m_Asset.reset();
        LoadAsset();
        m_PreviewAccumulator = 0.0f;
    }

    void ParticleEmitterEditorPanel::LoadAsset()
    {
        if (!Project::GetActive() || m_EmitterHandle == 0)
            return;
        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;
        Ref<Asset> ref = assetManager->GetAsset(m_EmitterHandle);
        if (ref)
            m_Asset = std::static_pointer_cast<ParticleEmitterAsset>(ref);
    }

    void ParticleEmitterEditorPanel::UpdatePreview(float deltaTime)
    {
        if (!m_Asset || !m_PreviewPlaying)
        {
            m_PreviewParticleSystem.OnUpdate(deltaTime);
            return;
        }
        m_PreviewAccumulator += deltaTime * m_Asset->EmissionRate;
        int emitCount = static_cast<int>(std::floor(m_PreviewAccumulator));
        if (emitCount > 0)
        {
            m_PreviewAccumulator -= static_cast<float>(emitCount);
            ParticleProps props = m_Asset->TemplateProps;
            props.position = glm::vec3(0.0f, 0.0f, 0.0f);
            for (int i = 0; i < emitCount; ++i)
                m_PreviewParticleSystem.Emit(props);
        }
        m_PreviewParticleSystem.OnUpdate(deltaTime);
    }

    void ParticleEmitterEditorPanel::RenderPreview()
    {
        ImVec2 region = ImGui::GetContentRegionAvail();
        if (region.x < 1.0f || region.y < 1.0f)
            return;

        // 固定 1:1 预览比例，保持正确缩放比：缩小窗口时用黑边而非压缩内容
        float displaySize = std::min(region.x, region.y);
        uint32_t fbSize = static_cast<uint32_t>(displaySize);
        if (fbSize < 1)
            fbSize = 1;
        if (m_Framebuffer->GetSpecification().Width != fbSize || m_Framebuffer->GetSpecification().Height != fbSize)
            m_Framebuffer->Resize(fbSize, fbSize);

        // 正交投影固定 1:1，世界 -5..5 始终对应正方形 viewport
        m_Camera.SetProjection(-5.0f, 5.0f, -5.0f, 5.0f);

        m_Framebuffer->Bind();
        RenderCommand::SetClearColor({ 0.12f, 0.12f, 0.15f, 1.0f });
        RenderCommand::Clear();

        Ref<Texture2D> previewTexture;
        if (m_Asset && m_Asset->TemplateProps.textureHandle != 0)
        {
            auto assetManager = ResourceSystem::GetAssetManager();
            if (assetManager && assetManager->IsAssetHandleValid(static_cast<AssetHandle>(m_Asset->TemplateProps.textureHandle)))
            {
                Ref<Asset> textureAsset = assetManager->GetAsset(static_cast<AssetHandle>(m_Asset->TemplateProps.textureHandle));
                previewTexture = std::dynamic_pointer_cast<Texture2D>(textureAsset);
            }
        }

        Renderer2D::BeginScene(m_Camera);
        m_PreviewParticleSystem.ForEachAlive([&](const ParticleSystem::ParticleView& p)
        {
            float t = 1.0f - p.remainingLife / p.lifetime;
            glm::vec4 color = glm::mix(p.colorBegin, p.colorEnd, t);
            float size = glm::mix(p.sizeBegin, p.sizeEnd, t);
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), p.position)
                * glm::rotate(glm::mat4(1.0f), p.rotation, glm::vec3(0, 0, 1))
                * glm::scale(glm::mat4(1.0f), glm::vec3(size));

            if (p.shape == ParticleShape::Circle)
                Renderer2D::DrawCircle(transform, color, 1.0f, 0.0025f);
            else
            {
                if (previewTexture)
                    Renderer2D::DrawQuad(transform, previewTexture, 1.0f, color);
                else
                    Renderer2D::DrawQuad(transform, color);
            }
        });
        Renderer2D::EndScene();

        m_Framebuffer->Unbind();

        // 在可用区域内居中显示正方形预览，保持 1:1 不拉伸
        float offsetX = (region.x - displaySize) * 0.5f;
        float offsetY = (region.y - displaySize) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);
        uint64_t texId = m_Framebuffer->GetColorAttachmentRendererID(0);
        ImGui::Image(reinterpret_cast<void*>(texId), ImVec2(displaySize, displaySize), { 0, 1 }, { 1, 0 });
    }

    void ParticleEmitterEditorPanel::UI_Properties()
    {
        if (!m_Asset)
            return;

        ParticleProps& particleProperties = m_Asset->TemplateProps;
        auto assetManager = ResourceSystem::GetAssetManager();

        std::string emitterAssetDisplayName = "Unknown";
        if (assetManager)
        {
            const auto& registry = assetManager->GetAssetRegistry();
            auto iterator = registry.find(m_EmitterHandle);
            if (iterator != registry.end())
                emitterAssetDisplayName = iterator->second.FilePath.filename().string();
        }
        DrawReadOnlyTextControl("Asset", emitterAssetDisplayName.c_str(),
                                "当前编辑的粒子发射器资产文件名。");

        DrawInspectorSectionHeader("Spawn Position (Template)",
                                   "Runtime emission uses the entity world position.");
        DrawVec3Control("Position", particleProperties.position, 0.0f);

        DrawInspectorSectionHeader("Initial Velocity");
        DrawVec3Control("Velocity", particleProperties.velocity, 0.0f);
        DrawVec3Control("Velocity Variation", particleProperties.velocityVariation, 0.0f);

        DrawInspectorSectionHeader("Lifetime");
        DrawFloatControl("Lifetime", particleProperties.lifetime, 0.05f, 0.01f, 10.0f);

        DrawInspectorSectionHeader("Shape");
        {
            const char* shapeLabels[] = {"Quad", "Circle"};
            int shapeIndex = static_cast<int>(particleProperties.shape);
            DrawEnumComboControl(
                "Shape", shapeIndex, shapeLabels, 2,
                [&](int newIndex)
                {
                    particleProperties.shape = static_cast<ParticleShape>(newIndex);
                });
        }

        {
            Ref<Texture2D> previewTexture;
            std::string textureDisplayName = "None (color only)";
            AssetHandle textureHandle = static_cast<AssetHandle>(particleProperties.textureHandle);
            if (textureHandle != 0 && assetManager
                && assetManager->IsAssetHandleValid(textureHandle))
            {
                const auto& registry = assetManager->GetAssetRegistry();
                auto iterator = registry.find(textureHandle);
                if (iterator != registry.end())
                    textureDisplayName = iterator->second.FilePath.filename().string();
                else
                    textureDisplayName = "Missing Texture";

                Ref<Asset> textureAsset = assetManager->GetAsset(textureHandle);
                if (textureAsset)
                    previewTexture = std::dynamic_pointer_cast<Texture2D>(textureAsset);
            }

            DrawObjectReferenceField(
                "Texture", textureDisplayName.c_str(), textureHandle != 0, previewTexture,
                [&]()
                {
                    particleProperties.textureHandle = 0;
                },
                [&](const ImGuiPayload* payload)
                {
                    Ref<Texture2D> assignedTexture;
                    AssetHandle assignedHandle = 0;
                    if (!AssignTextureFromContentBrowserPayload(payload, assignedTexture, assignedHandle))
                        return false;
                    particleProperties.textureHandle = static_cast<uint64_t>(assignedHandle);
                    return true;
                });
        }

        DrawInspectorSectionHeader("Appearance");
        DrawColorControl("Color Begin", particleProperties.colorBegin);
        DrawColorControl("Color End", particleProperties.colorEnd);
        DrawFloatControl("Size Begin", particleProperties.sizeBegin, 0.01f, 0.0f, 5.0f);
        DrawFloatControl("Size End", particleProperties.sizeEnd, 0.01f, 0.0f, 5.0f);

        DrawInspectorSectionHeader("Emitter");
        DrawFloatControl("Emission Rate", m_Asset->EmissionRate, 1.0f, 0.0f, 500.0f);
        DrawCheckboxControl("Looping", m_Asset->Looping);
    }

    void ParticleEmitterEditorPanel::SaveAsset()
    {
        if (!m_Asset || m_EmitterHandle == 0 || !Project::GetActive())
            return;
        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;
        const auto& registry = assetManager->GetAssetRegistry();
        auto it = registry.find(m_EmitterHandle);
        if (it != registry.end())
        {
            std::filesystem::path fullPath = Project::GetAssetFileSystemPath(it->second.FilePath);
            ParticleEmitterAssetSerializer::Serialize(fullPath, m_Asset);
            HIMII_CORE_INFO("ParticleEmitter saved to: {0}", fullPath.string());
        }
    }

    void ParticleEmitterEditorPanel::OnImGuiRender(bool& isOpen)
    {
        if (!isOpen)
            return;

        const float minWinW = 6.0f + MinPreviewWidth + MinPropertiesPanelWidth;
        ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(minWinW, 200.0f), ImVec2(FLT_MAX, FLT_MAX));
        if (!ImGui::Begin("Particle Emitter Editor", &isOpen))
        {
            ImGui::End();
            return;
        }

        if (m_EmitterHandle == 0 || !m_Asset)
        {
            ImGui::TextDisabled("No emitter loaded.");
            ImGui::TextDisabled("Select an entity with ParticleEmitterComponent and click 'Open in Particle Emitter Editor'.");
            ImGui::End();
            return;
        }

        UpdatePreview(ImGui::GetIO().DeltaTime);

        DrawActionButtonRow("Actions", [&]()
        {
            if (ImGui::Button("Save", ImVec2(80.0f, 0.0f)))
                SaveAsset();
            ImGui::SameLine();
            if (ImGui::Button(m_PreviewPlaying ? "Pause" : "Play", ImVec2(80.0f, 0.0f)))
                m_PreviewPlaying = !m_PreviewPlaying;
        });
        ImGui::Separator();

        const float splitterWidth = 6.0f;
        float totalWidth = ImGui::GetContentRegionAvail().x;
        if (totalWidth < splitterWidth + MinPropertiesPanelWidth + MinPreviewWidth)
            totalWidth = splitterWidth + MinPropertiesPanelWidth + MinPreviewWidth;

        m_PropertiesPanelWidth = std::clamp(m_PropertiesPanelWidth,
            MinPropertiesPanelWidth,
            totalWidth - splitterWidth - MinPreviewWidth);
        float previewWidth = totalWidth - splitterWidth - m_PropertiesPanelWidth;

        ImGui::BeginChild("Preview", ImVec2(previewWidth, -1), true, ImGuiWindowFlags_NoScrollbar);
        RenderPreview();
        ImGui::EndChild();

        ImGui::SameLine(0, 0);

        ImGui::InvisibleButton("##Splitter", ImVec2(splitterWidth, -1));
        bool splitterHovered = ImGui::IsItemHovered();
        if (ImGui::IsItemActive())
        {
            m_PropertiesPanelWidth -= ImGui::GetIO().MouseDelta.x;
            m_PropertiesPanelWidth = std::clamp(m_PropertiesPanelWidth,
                MinPropertiesPanelWidth,
                totalWidth - splitterWidth - MinPreviewWidth);
        }
        if (splitterHovered)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        // 分割条竖线
        ImVec2 sMin = ImGui::GetItemRectMin();
        ImVec2 sMax = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRectFilled(sMin, sMax,
            splitterHovered ? IM_COL32(100, 100, 100, 255) : IM_COL32(60, 60, 60, 255));

        ImGui::SameLine(0, 0);

        ImGui::BeginChild("Properties", ImVec2(m_PropertiesPanelWidth, -1), true);
        UI_Properties();
        ImGui::EndChild();

        ImGui::End();
    }
}
