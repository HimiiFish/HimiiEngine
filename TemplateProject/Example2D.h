#pragma once
#include "Engine.h"

class Example2D: public Himii::Layer
{
	public:
		Example2D();
        virtual ~Example2D() = default;

		virtual void OnAttach() override;
        virtual void OnDetach() override;

        virtual void OnUpdate(Himii::Timestep ts) override;
        virtual void OnImGuiRender() override;
        virtual void OnEvent(Himii::Event &event) override;

    private:
    // ECS 驱动场景
    Himii::Scene m_Scene;
	private:
        Himii::OrthographicCameraController m_CameraController;
        
    // 可选：后续可扩展纹理/渲染资源，这里最小示例仅用颜色方块

    // Scene 视口用的离屏帧缓冲
    Himii::Ref<Himii::Framebuffer> m_SceneFramebuffer;

    glm::vec4 m_SquareColor = {0.5f, 0.26f, 0.56f,1.0f};
};