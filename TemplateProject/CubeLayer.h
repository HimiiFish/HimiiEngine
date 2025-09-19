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
    // 简单的图集参数：按网格切分
    struct AtlasTile {
        int col = 0;
        int row = 0;
    };
    static glm::vec4 AtlasUVRect(int col, int row, int cols, int rows, float padding = 0.0f);

    void RebuildVB();
    void UpdateCamera(Himii::Timestep ts);
    void BuildTerrainMesh();

private:
    Himii::Ref<Himii::VertexArray> m_CubeVA;     // 旧：单方块（保留但不再使用）
    Himii::Ref<Himii::VertexArray> m_TerrainVA;  // 新：整张地形网格
    Himii::Ref<Himii::Texture2D>   m_Atlas;
    Himii::Ref<Himii::Shader>      m_TextureShader;
    Himii::ShaderLibrary           m_ShaderLibrary;

    // 图集网格（可在 ImGui 中调整）
    int m_AtlasCols = 10;
    int m_AtlasRows = 16;
    float m_GridPadding = 0.0f; // 归一化 padding（0..1/cols, 0..1/rows 的尺度）

    // 旧单方块字段已废弃

    float m_RotateSpeed = 45.0f; // 度/秒
    float m_Angle = 0.0f;

    // 透视相机参数（替换原 OrthographicCameraController 的使用）
    // 说明：我们在层内自己构建 View/Projection 矩阵并通过 Renderer::BeginScene(mat4) 提交
    float m_FovYDeg = 45.0f;
    float m_NearZ = 0.1f;
    float m_FarZ = 100.0f;
    glm::vec3 m_CamPos{0.0f, 0.0f, 3.0f};
    glm::vec3 m_CamTarget{0.0f, 0.0f, 0.0f};
    glm::vec3 m_CamUp{0.0f, 1.0f, 0.0f};
    float m_Aspect = 1280.0f / 720.0f; // 初值，窗口回调时可更新

    // 相机输入控制
    float m_MoveSpeed = 3.0f;           // 单位/秒
    float m_MouseSensitivity = 0.1f;    // 度/像素
    float m_YawDeg = -90.0f;            // 初始朝向 -Z
    float m_PitchDeg = 0.0f;
    bool  m_MouseLook = false;          // 是否正在进行鼠标观察（按住右键）
    bool  m_OrientInitialized = false;  // 从 CamPos/Target 初始化一次 yaw/pitch
    float m_LastMouseX = 0.0f;
    float m_LastMouseY = 0.0f;

    // 地形参数（体素体积大小）
    int m_TerrainW = 30;
    int m_TerrainD = 30;
    int m_TerrainH = 10;

    // 各方块类型的 UV 映射（留空供你按图集填写）
    struct BlockUV { AtlasTile top{}, bottom{}, side{}; };
    BlockUV m_GrassUV{{0, 0}, {0, 0}, {0, 0}}; // 草方块：top/side/bottom
    BlockUV m_DirtUV{{0, 0}, {0, 0}, {0, 0}};  // 泥土：一般三面相同，可只用 side
    BlockUV m_StoneUV{{0, 0}, {0, 0}, {0, 0}}; // 石头：三面相同，可只用 side
};