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
        // 可选：TileMap 渲染（行列 IDs）
        void DrawTileMapDemo();
	private:
        Himii::OrthographicCameraController m_CameraController;

        Himii::Ref<Himii::Shader> m_Shader;
        Himii::Ref<Himii::VertexArray> m_SquareVA;
        Himii::Ref<Himii::Texture2D> m_GrassTexture;
        Himii::Ref<Himii::Texture2D> m_MudTexture;
    // 图集纹理（blocks.png）
    Himii::Ref<Himii::Texture2D> m_BlocksAtlas;
    int m_AtlasCols = 10; // blocks.png 的列数（可根据实际修改）
    int m_AtlasRows = 16; // blocks.png 的行数（可根据实际修改）
    float m_AtlasPad = 0.0f; // 归一化 padding（避免采样溢出，可设 0.001f 等）

    // Scene 视口用的离屏帧缓冲
    Himii::Ref<Himii::Framebuffer> m_SceneFramebuffer;

        glm::vec4 m_SquareColor = {0.5f, 0.26f, 0.56f,1.0f};

        Himii::Ref<Terrain> m_Terrain;
    bool m_UseTileMap = false; // 可在 ImGui 中切换 Terrain/TileMap
};