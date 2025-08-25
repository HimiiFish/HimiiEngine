#pragma once
#include "Engine.h"
#include "glm/glm.hpp"
// 使用编辑器相机（仅用于 Scene 视口渲染与拾取）
#include "Himii/Renderer/EditorCamera.h"

class CubeLayer : public Himii::Layer
{
public:
    CubeLayer();
    ~CubeLayer() override = default;
    friend class EditorLayer; // Inspector 直接管理场景参数与重建

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Himii::Timestep ts) override;
    virtual void OnEvent(Himii::Event& e) override;

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
    void BuildSkybox();

    // ECS 化：将当前 VAO/Shader 等包装到 ECS 中，便于压力测试
    void BuildECSScene();

private:
    Himii::Ref<Himii::VertexArray> m_CubeVA;     // 旧：单方块（保留但不再使用）
    Himii::Ref<Himii::VertexArray> m_TerrainVA;  // 新：整张地形网格
    Himii::Ref<Himii::VertexArray> m_SkyboxVA;   // 天空盒立方体
    Himii::Ref<Himii::Texture2D>   m_Atlas;
    Himii::Ref<Himii::Shader>      m_TextureShader; // 旧 2D 纹理着色器
    Himii::Ref<Himii::Shader>      m_LitShader;     // 新：带全局光照
    Himii::Ref<Himii::Shader>      m_SkyboxShader;  // 新：天空盒
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

    // 地形参数已迁移到 TerrainScript（通过 NativeScriptComponent 挂在地形实体上）

    // 全局光照参数
    glm::vec3 m_AmbientColor {0.35f, 0.40f, 0.45f};
    float     m_AmbientIntensity = 0.4f;
    glm::vec3 m_LightDir{-0.4f, -1.0f, -0.3f};
    glm::vec3 m_LightColor{1.0f, 0.98f, 0.92f};
    float     m_LightIntensity = 1.0f;

        // 噪声参数（fBm + Ridged + Domain Warp，可在 ImGui 中调整）
        struct NoiseSettings {
            uint32_t seed = 1337;
            // 生物群落混合（低频）
            float biomeScale = 0.02f; // 越小越大片区
            // 大尺度大陆感
            float continentScale = 0.008f;   // 大地貌尺度（越小越大块）
            float continentStrength = 0.6f;  // 对高度的影响强度 0..1
            // 平原（fBm）
            float plainsScale = 0.10f;
            int    plainsOctaves = 5;
            float plainsLacunarity = 2.0f;
            float plainsGain = 0.5f;
            // 山地（Ridged）
            float mountainScale = 0.06f;
            int    mountainOctaves = 5;
            float mountainLacunarity = 2.0f;
            float mountainGain = 0.5f;
            float ridgeSharpness = 1.5f;     // 山脊锐度（>1 更尖）
            // Domain Warp（坐标扰动）
            float warpScale = 0.15f;
            float warpAmp = 2.5f;
            // 细节层
            float detailScale = 0.25f;       // 细节频率
            float detailAmp = 0.15f;         // 细节振幅 0..1
            // 高度整形
            float heightMul = 1.0f;  // 相对 H 的倍数（最终再乘以 m_TerrainH-1）
            float plateau = 0.15f;   // 平滑台地强度 [0..1]
            int   stepLevels = 4;     // 台地层数 >=1
            float curveExponent = 1.1f; // 高度分布指数（>1 提升高区占比）
            float valleyDepth = 0.05f; // 谷地下切（0..0.4）
            float seaLevel = 0.0f;   // 0..1 归一海平面
            float mountainWeight = 0.56f; // 群落混合偏向山地
        } m_Noise;

    // 各方块类型的 UV 映射（留空供你按图集填写）
    struct BlockUV { AtlasTile top{}, bottom{}, side{}; };
    BlockUV m_GrassUV{{2, 12}, {1, 14}, {1, 12}}; // 草方块：top/side/bottom
    BlockUV m_DirtUV{{1, 13}, {1, 13}, {1, 13}};  // 泥土：一般三面相同，可只用 side
    BlockUV m_StoneUV{{7, 8}, {7, 8}, {7, 8}}; // 石头：三面相同，可只用 side

    // 离屏渲染
    Himii::Ref<Himii::Framebuffer> m_Framebuffer;
    Himii::Ref<Himii::Framebuffer> m_GameFramebuffer;
    Himii::Ref<Himii::Framebuffer> m_PickingFramebuffer; // R32UI picking buffer

    // ECS 场景，用于替换手写 Submit 流程
    Himii::Scene m_Scene;
    // 记录 ECS 中的实体句柄（便于重建/替换 VAO）
    Himii::Entity m_TerrainEntity;
    Himii::Entity m_SkyboxEntity;
    // entity id mapping for picking
    uint32_t m_TerrainPickID = 1; // assign stable ids per entity
    uint32_t m_SkyboxPickID = 2;

    // 编辑器相机（驱动 Scene 视口与拾取）
    Himii::EditorCamera m_EditorCamera;
};