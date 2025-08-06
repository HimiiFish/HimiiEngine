#pragma once
#include "Engine.h"
#include "Terrain.h"

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
        Himii::OrthographicCameraController m_CameraController;

        Himii::Ref<Himii::Shader> m_Shader;
        Himii::Ref<Himii::VertexArray> m_SquareVA;
        Himii::Ref<Himii::Texture2D> m_BlockTexture;
        glm::vec4 m_SquareColor = {0.5f, 0.26f, 0.56f,1.0f};

        Himii::Ref<Terrain> m_Terrain;
};