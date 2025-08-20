#pragma once
#include "Engine.h"
#include "glm/glm.hpp"

class CubeLayer : public Himii::Layer
{
public:
    CubeLayer();
    ~CubeLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Himii::Timestep ts) override;
    void OnImGuiRender() override;
    void OnEvent(Himii::Event& e) override;

private:
    enum class AtlasMode { Grid, Pixel };
    // 简单的图集参数：按网格切分
    struct AtlasTile {
        int col = 0;
        int row = 0;
    };
    static glm::vec4 AtlasUVRect(int col, int row, int cols, int rows, float padding = 0.0f);

    // 像素矩形模式（单位：像素）
    struct AtlasRect { int x=0, y=0, w=0, h=0; };
    static glm::vec4 AtlasUVRectPixels(int x, int y, int w, int h, int atlasW, int atlasH, bool originTopLeft);

    void RebuildVB();

private:
    Himii::OrthographicCameraController m_CameraController;

    Himii::Ref<Himii::VertexArray> m_CubeVA;
    Himii::Ref<Himii::Texture2D>   m_Atlas;
    Himii::Ref<Himii::Shader>      m_TextureShader;
    Himii::ShaderLibrary           m_ShaderLibrary;

    // 图集网格（可在 ImGui 中调整）
    int m_AtlasCols = 4;
    int m_AtlasRows = 4;
    float m_GridPadding = 0.0f; // 归一化 padding（0..1/cols, 0..1/rows 的尺度）

    // 各面的图集索引（默认用同一张子贴图）
    AtlasTile m_Front{0, 0};
    AtlasTile m_Back{0, 0};
    AtlasTile m_Left{0, 0};
    AtlasTile m_Right{0, 0};
    AtlasTile m_Top{0, 0};
    AtlasTile m_Bottom{0, 0};

    // 像素矩形参数（需要手动填写图集尺寸与各面矩形）
    int m_AtlasWidth = 1024;
    int m_AtlasHeight = 1024;
    bool m_OriginTopLeft = true; // 大多数外部图集以左上为原点
    AtlasRect m_FrontPx{0,0,256,256};
    AtlasRect m_BackPx{0,0,256,256};
    AtlasRect m_LeftPx{0,0,256,256};
    AtlasRect m_RightPx{0,0,256,256};
    AtlasRect m_TopPx{0,0,256,256};
    AtlasRect m_BottomPx{0,0,256,256};

    AtlasMode m_Mode = AtlasMode::Grid;

    float m_RotateSpeed = 45.0f; // 度/秒
    float m_Angle = 0.0f;
};