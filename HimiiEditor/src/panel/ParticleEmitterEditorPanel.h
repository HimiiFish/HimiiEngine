#pragma once

#include "EngineCore/Core/Core.h"
#include "Resource/Asset.h"
#include "Module/Render/Renderer/OrthographicCamera.h"
#include "Module/Render/RenderCore/Framebuffer.h"
#include "Module/Particle/ParticleEmitterAsset.h"
#include "Module/Particle/ParticleSystem.h"

#include <glm/glm.hpp>

namespace Himii
{
    class ParticleEmitterEditorPanel
    {
    public:
        // 分割条缩放限制：按需修改以放宽或收紧预览/属性区最小宽度
        static constexpr float MinPropertiesPanelWidth = 260.0f;  // 右侧属性面板最小宽度
        static constexpr float MinPreviewWidth = 120.0f;         // 左侧预览区最小宽度

        ParticleEmitterEditorPanel();

        void OnImGuiRender(bool& isOpen);
        void Open(AssetHandle emitterHandle);

    private:
        void LoadAsset();
        void UpdatePreview(float deltaTime);
        void RenderPreview();
        void UI_Properties();
        void SaveAsset();

        Ref<Framebuffer> m_Framebuffer;
        OrthographicCamera m_Camera;
        glm::vec2 m_PreviewSize{ 320.0f, 240.0f };

        AssetHandle m_EmitterHandle = 0;
        Ref<ParticleEmitterAsset> m_Asset;

        ParticleSystem m_PreviewParticleSystem{ 5000 };
        bool m_PreviewPlaying = true;
        float m_PreviewAccumulator = 0.0f;

        // 可拖拽分割：右侧属性面板宽度，用于调整预览与属性区比例
        float m_PropertiesPanelWidth = 260.0f;
    };
}
