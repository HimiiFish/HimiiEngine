#include "CubeLayer.h"
#include <algorithm>
#include <array>
#include "EditorLayer.h"
#include "Himii/Core/Application.h"
#include "Himii/Core/Log.h"
#include "Himii/Events/ApplicationEvent.h"
#include "Himii/Renderer/Framebuffer.h"
#include "Himii/Renderer/RenderCommand.h"
#include "Terrain.h"
#include "glad/glad.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "TerrainScript.h"
// #include "imgui.h" // moved all UI to EditorLayer Inspector

CubeLayer::CubeLayer() : Himii::Layer("CubeLayer")
{
}

glm::vec4 CubeLayer::AtlasUVRect(int col, int row, int cols, int rows, float padding)
{
    // 注意坐标系：这里默认贴图原点在左下（常见做法是stbi翻转），col/row 从左下角开始计数
    const float du = 1.0f / cols;
    const float dv = 1.0f / rows;

    float u0 = col * du + padding;
    float v0 = row * dv + padding;
    float u1 = (col + 1) * du - padding;
    float v1 = (row + 1) * dv - padding;
    return {u0, v0, u1, v1};
}

// 像素矩形模式已移除，保留网格模式接口

void CubeLayer::OnAttach()
{
    // 开启深度测试（如全局已开启可省略）
    glEnable(GL_DEPTH_TEST);
    // 背面剔除：为排查可见性问题，先禁用（确认正常后可再开启并校正三角形绕序）
    glDisable(GL_CULL_FACE);

    // 1) 加载着色器与纹理资源
    {
        // 着色器位于 TemplateProject/assets/shaders
        m_LitShader = m_ShaderLibrary.Load("assets/shaders/LitTexture.glsl");
        m_SkyboxShader = m_ShaderLibrary.Load("assets/shaders/Skybox.glsl");

        // 纹理图集（体素贴图）
        m_Atlas = Himii::Texture2D::Create("assets/textures/blocks.png");

        // 初始化采样器数组（LitTexture.glsl 使用 u_Texture[32]）
        if (m_LitShader)
        {
            int samplers[32];
            for (int i = 0; i < 32; ++i) samplers[i] = i;
            m_LitShader->Bind();
            m_LitShader->SetIntArray("u_Texture", samplers, 32);
        }
    }

    // 2) 生成网格：地形与天空盒
    BuildTerrainMesh();
    BuildSkybox();
    // 3) 绑定到 ECS 实体，供 Scene::OnUpdate 渲染
    BuildECSScene();

    // 4) 将当前场景注入 Editor，确保 Hierarchy/Inspector 可见
    if (auto *editor = dynamic_cast<EditorLayer*>(Himii::Application::Get().GetLayerStack().begin()!=Himii::Application::Get().GetLayerStack().end() ? *Himii::Application::Get().GetLayerStack().begin() : nullptr))
    {
        if (editor && editor->GetActiveScene() != &m_Scene)
            editor->SetActiveScene(&m_Scene);
    }
    else
    {
        // 遍历查找 EditorLayer（更稳妥）
        for (auto *layer : Himii::Application::Get().GetLayerStack())
            if (auto *ed = dynamic_cast<EditorLayer*>(layer)) { ed->SetActiveScene(&m_Scene); break; }
    }
}

void CubeLayer::OnDetach()
{
    HIMII_PROFILE_FUNCTION();
}

void CubeLayer::OnUpdate(Himii::Timestep ts)
{
    // 离屏渲染由下方 renderTo 分别处理 Scene/Game

    // 从 EditorLayer 获取 Scene 期望尺寸，驱动 FBO Resize 和相机宽高
    Himii::Application &appRef = Himii::Application::Get();
    EditorLayer *editorRef = nullptr;
    for (auto *layer: appRef.GetLayerStack())
        if ((editorRef = dynamic_cast<EditorLayer *>(layer)))
            break;
    // 确保 EditorLayer 持有当前场景（避免启动顺序导致的空指针）
    if (editorRef && editorRef->GetActiveScene() != &m_Scene)
        editorRef->SetActiveScene(&m_Scene);
    uint32_t sceneW = 0, sceneH = 0, gameW = 0, gameH = 0;
    float aspectScene = m_Aspect, aspectGame = m_Aspect;
    if (editorRef && m_Framebuffer)
    {
        ImVec2 desired = editorRef->GetSceneDesiredSize();
        uint32_t newW = (uint32_t)std::max(1.0f, desired.x);
        uint32_t newH = (uint32_t)std::max(1.0f, desired.y);
        auto &spec = m_Framebuffer->GetSpecification();
        if (newW != spec.Width || newH != spec.Height)
        {
            m_Framebuffer->Resize(newW, newH);
            if (m_PickingFramebuffer) m_PickingFramebuffer->Resize(newW, newH);
            aspectScene = (float)newW / (float)newH;
            // 同步编辑器相机 viewport
            m_EditorCamera.SetViewport((float)newW, (float)newH);
            // 同步场景内摄像机的宽高比，避免拉伸变形
            auto camView = m_Scene.Registry().view<Himii::CameraComponent>();
            for (auto e : camView) {
                auto &cc = camView.get<Himii::CameraComponent>(e);
                if (cc.projection == Himii::ProjectionType::Perspective)
                    cc.camera.SetPerspective(cc.fovYDeg, aspectScene, cc.nearZ, cc.farZ);
                else {
                    float orthoH = 10.0f; // 以 10 单位为基准高度
                    float orthoW = orthoH * aspectScene;
                    cc.camera.SetOrthographic(-orthoW*0.5f, orthoW*0.5f, -orthoH*0.5f, orthoH*0.5f, cc.nearZ, cc.farZ);
                }
            }
        }
        sceneW = m_Framebuffer->GetSpecification().Width;
        sceneH = m_Framebuffer->GetSpecification().Height;
    }
    if (editorRef && m_GameFramebuffer)
    {
        ImVec2 desired = editorRef->GetGameDesiredSize();
        uint32_t newW = (uint32_t)std::max(1.0f, desired.x);
        uint32_t newH = (uint32_t)std::max(1.0f, desired.y);
        auto &spec = m_GameFramebuffer->GetSpecification();
        if (newW != spec.Width || newH != spec.Height)
        {
            m_GameFramebuffer->Resize(newW, newH);
            aspectGame = (float)newW / (float)newH;
        }
        gameW = m_GameFramebuffer->GetSpecification().Width;
        gameH = m_GameFramebuffer->GetSpecification().Height;
    }

    // 相机输入更新：仅在 Scene 面板聚焦/悬停时接收输入，驱动编辑器相机姿态
    if (!editorRef || editorRef->IsSceneHovered() || editorRef->IsSceneFocused())
        UpdateCamera(ts);

    // 渲染 Scene 视口：使用编辑器相机（外部 VP 覆盖）
    if (m_Framebuffer)
    {
        m_Framebuffer->Bind();
        Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
        Himii::RenderCommand::Clear();

        // 用编辑器相机提供的 VP
        glm::mat4 viewProj = m_EditorCamera.GetViewProjection();

        // 天空盒跟随编辑器相机
        if (m_SkyboxEntity)
        {
            auto &tr = m_SkyboxEntity.GetComponent<Himii::Transform>();
            tr.Position = m_CamPos;
            tr.Scale = glm::vec3(m_FarZ * 0.5f);
            tr.Rotation = {0.0f, glm::radians(m_Angle), 0.0f};
        }

        if (m_LitShader)
        {
            m_LitShader->Bind();
            m_LitShader->SetFloat3("u_AmbientColor", m_AmbientColor);
            m_LitShader->SetFloat("u_AmbientIntensity", m_AmbientIntensity);
            m_LitShader->SetFloat3("u_LightDir", m_LightDir);
            m_LitShader->SetFloat3("u_LightColor", m_LightColor);
            m_LitShader->SetFloat("u_LightIntensity", m_LightIntensity);
        }

        // 使用外部 VP 覆盖渲染 Scene
        m_Scene.SetExternalViewProjection(&viewProj);
        m_Scene.OnUpdate({});
        m_Scene.SetExternalViewProjection(nullptr);

        m_Framebuffer->Unbind();
    }

    // 渲染 Game 视口：使用场景中的 CameraComponent（运行时相机）
    if (m_GameFramebuffer)
    {
        m_GameFramebuffer->Bind();
        Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
        Himii::RenderCommand::Clear();

        // 天空盒跟随主摄像机（若存在）
        entt::entity camEntity = entt::null;
        auto viewCam = m_Scene.Registry().view<Himii::Transform, Himii::CameraComponent>();
        if (m_SkyboxEntity && camEntity != entt::null)
        {
            auto &trSky = m_SkyboxEntity.GetComponent<Himii::Transform>();
            auto &trCam = viewCam.get<Himii::Transform>(camEntity);
            auto &cc = viewCam.get<Himii::CameraComponent>(camEntity);
            trSky.Position = trCam.Position;
            float farZ = cc.farZ > 0.0f ? cc.farZ : m_FarZ;
            trSky.Scale = glm::vec3(farZ * 0.5f);
            trSky.Rotation = {0.0f, glm::radians(m_Angle), 0.0f};
        }

        if (m_LitShader)
        {
            m_LitShader->Bind();
            m_LitShader->SetFloat3("u_AmbientColor", m_AmbientColor);
            m_LitShader->SetFloat("u_AmbientIntensity", m_AmbientIntensity);
            m_LitShader->SetFloat3("u_LightDir", m_LightDir);
            m_LitShader->SetFloat3("u_LightColor", m_LightColor);
            m_LitShader->SetFloat("u_LightIntensity", m_LightIntensity);
        }

        // 不设置外部 VP，场景内部将选用 CameraComponent 渲染
        m_Scene.SetExternalViewProjection(nullptr);
        m_Scene.OnUpdate({});

        m_GameFramebuffer->Unbind();
    }

    // Picking pass: render only entity IDs into R32UI attachment
    if (m_PickingFramebuffer)
    {
        m_PickingFramebuffer->Bind();
    // Clear picking attachment to 0 (no entity)
    GLuint clearZero[4] = { 0, 0, 0, 0 };
    glClearBufferuiv(GL_COLOR, 1, clearZero);
        Himii::RenderCommand::Clear();

    // matrices: use the same editor camera as Scene viewport
    glm::mat4 viewProj = m_EditorCamera.GetViewProjection();

        static Himii::Ref<Himii::Shader> s_Picking;
        if (!s_Picking) s_Picking = m_ShaderLibrary.Load("assets/shaders/Picking.glsl");
        if (s_Picking)
        {
            // Skybox not drawn into picking
            auto meshView = m_Scene.Registry().view<Himii::Transform, Himii::MeshRenderer>(entt::exclude<Himii::SkyboxTag>);
            s_Picking->Bind();
            s_Picking->SetMat4("u_ViewProjection", viewProj);
        }
        m_PickingFramebuffer->Unbind();

        // If mouse over Scene panel, read pixel and set selection
        if (editorRef && (editorRef->IsSceneHovered() || editorRef->IsSceneFocused()))
        {
            uint32_t px=0, py=0;
            uint32_t texW = m_PickingFramebuffer->GetSpecification().Width;
            uint32_t texH = m_PickingFramebuffer->GetSpecification().Height;
            // Trigger selection on left mouse click within Scene image
            if (editorRef->GetSceneMousePixel(texW, texH, px, py) && Himii::Input::IsMouseButtonPressed(Himii::Mouse::ButtonLeft))
            {
            }
        }
    }

    // 若脚本标记 Dirty，则在这一帧末重建网格
    if (m_TerrainEntity && m_TerrainEntity.HasComponent<Himii::NativeScriptComponent>())
    {
        auto &nsc = m_TerrainEntity.GetComponent<Himii::NativeScriptComponent>();
        if (nsc.Instance)
        {
            if (auto *tsc = dynamic_cast<TerrainScript *>(nsc.Instance))
            {
                if (tsc->Dirty)
                {
                    BuildTerrainMesh();
                    auto &mr = m_TerrainEntity.GetComponent<Himii::MeshRenderer>();
                    mr.vertexArray = m_TerrainVA;
                    tsc->Dirty = false;
                }
            }
        }
    }
    // 这里简单地遍历 LayerStack 找到 EditorLayer 并传值
    auto &app2 = Himii::Application::Get();
    for (auto *layer: app2.GetLayerStack())
    {
        if (auto *editor = dynamic_cast<EditorLayer *>(layer))
        {
            if (m_Framebuffer)
            {
                editor->SetSceneTexture(m_Framebuffer->GetColorAttachmentRendererID());
                editor->SetSceneSize(m_Framebuffer->GetSpecification().Width, m_Framebuffer->GetSpecification().Height);
            }
            if (m_GameFramebuffer)
            {
                editor->SetGameTexture(m_GameFramebuffer->GetColorAttachmentRendererID());
                editor->SetGameSize(m_GameFramebuffer->GetSpecification().Width,
                                    m_GameFramebuffer->GetSpecification().Height);
            }
        }
    }
}

// 该层不再承担 ImGui 参数调试，改由 EditorLayer 的 Inspector 面板管理
void CubeLayer::OnEvent(Himii::Event &e)
{
    using namespace Himii;
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>(
            [&](WindowResizeEvent &ev)
            {
                if (ev.GetHeight() > 0)
                    m_Aspect = (float)ev.GetWidth() / (float)ev.GetHeight();
                return false;
            });
}
void CubeLayer::UpdateCamera(Himii::Timestep ts)
{
    using namespace Himii;
    // 初始化一次 yaw/pitch 以匹配当前 CamTarget
    if (!m_OrientInitialized)
    {
    // 从当前字段推导初始朝向（默认沿 -Z）
    glm::vec3 dir = glm::normalize(m_CamTarget - m_CamPos);
        // yaw: -Z 为 0? 我们以 -Z 为前方，初始 m_YawDeg=-90 指向 -Z
        m_PitchDeg = glm::degrees(asinf(glm::clamp(dir.y, -1.0f, 1.0f)));
        m_YawDeg = glm::degrees(atan2f(dir.z, dir.x));
        m_OrientInitialized = true;
        // 初始化鼠标位置
        glm::vec2 m = Input::GetMousePosition();
        m_LastMouseX = m.x;
        m_LastMouseY = m.y;
    }

    // 鼠标右键按住时开启鼠标观察
    if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
    {
        glm::vec2 cur = Input::GetMousePosition();
        float dx = cur.x - m_LastMouseX;
        float dy = cur.y - m_LastMouseY;
        m_LastMouseX = cur.x;
        m_LastMouseY = cur.y;

        m_YawDeg += dx * m_MouseSensitivity;
        m_PitchDeg -= dy * m_MouseSensitivity; // 屏幕向下为正，俯仰取负
        m_PitchDeg = glm::clamp(m_PitchDeg, -89.0f, 89.0f);
    }
    else
    {
        // 未按住时，仅更新基准，上一次位置对齐以避免下次跳变
        glm::vec2 cur = Input::GetMousePosition();
        m_LastMouseX = cur.x;
        m_LastMouseY = cur.y;
    }

    // 根据 yaw/pitch 计算前向向量（右手坐标，OpenGL 风格）
    float yawR = glm::radians(m_YawDeg);
    float pitchR = glm::radians(m_PitchDeg);
    glm::vec3 front;
    front.x = cosf(pitchR) * cosf(yawR);
    front.y = sinf(pitchR);
    front.z = cosf(pitchR) * sinf(yawR);
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    // 键盘 WASD 移动
    float dt = (float)ts;
    float speed = m_MoveSpeed;
    if (Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift))
        speed *= 2.0f;

    glm::vec3 pos = m_CamPos;
    if (Input::IsKeyPressed(Key::W)) pos += front * speed * dt;
    if (Input::IsKeyPressed(Key::S)) pos -= front * speed * dt;
    if (Input::IsKeyPressed(Key::A)) pos -= right * speed * dt;
    if (Input::IsKeyPressed(Key::D)) pos += right * speed * dt;
    if (Input::IsKeyPressed(Key::Q)) pos -= up * speed * dt;
    if (Input::IsKeyPressed(Key::E)) pos += up * speed * dt;

    // 更新编辑器相机姿态（用 LookAt 确保前向一致，W 前进）
    m_CamPos = pos;
    m_CamTarget = m_CamPos + front;
    m_CamUp = up;
    m_EditorCamera.SetLookAt(m_CamPos, m_CamTarget, m_CamUp);
}

// OnImGuiRender 已移除

void CubeLayer::BuildTerrainMesh()
{
    // 为避免缓冲区尺寸不足导致 glBufferSubData 失败，这里直接重建 VA/VB/IB
    m_TerrainVA = Himii::VertexArray::Create();

    // 从脚本读取地形大小与噪声参数（回退到旧默认）
    int W = 128, D = 128, H = 32;
    auto noise = m_Noise; // 使用现有字段作为默认初值
    if (m_TerrainEntity && m_TerrainEntity.HasComponent<Himii::NativeScriptComponent>())
    {
        auto &nsc = m_TerrainEntity.GetComponent<Himii::NativeScriptComponent>();
        if (nsc.Instance)
        {
            if (auto *ts = dynamic_cast<TerrainScript *>(nsc.Instance))
            {
                W = std::max(1, ts->Width);
                D = std::max(1, ts->Depth);
                H = std::max(1, ts->Height);
                //noise = ts->Noise;
            }
        }
    }

    PerlinNoise perlin(noise.seed);
    std::vector<int> heightMap(W * D, 0);
    for (int z = 0; z < D; ++z)
    {
        for (int x = 0; x < W; ++x)
        {
            // 归一坐标 [0,1]
            double nx = (double)x / (double)W;
            double nz = (double)z / (double)D;

            // 小幅 domain warp 去除网格感
            double wx = nx, wz = nz;
            perlin.domainWarp2D(wx, wz, (double)noise.warpScale, (double)noise.warpAmp);

            // 低频群落权重（0..1），调制山地比重
            double biome = perlin.fbm2D(nx * (double)noise.biomeScale, nz * (double)noise.biomeScale, 3, 2.0, 0.5);
            biome = std::clamp(biome * (double)noise.mountainWeight, 0.0, 1.0);

            // 大洲层：决定“大陆-海洋”的宏观起伏
            double continent = perlin.fbm2D(nx * (double)noise.continentScale, nz * (double)noise.continentScale, 4,
                                            2.0, 0.5); // 0..1
            continent = (continent * 2.0 - 1.0);       // [-1,1]
            continent = std::clamp(continent * (double)noise.continentStrength, -1.0, 1.0);
            // 映射至 0..1，正值抬升，负值压低
            double continentLift = (continent + 1.0) * 0.5; // 0..1

            // 平原（柔和）与山地（脊状）两套噪声
            double plains = perlin.fbm2D(wx * (double)noise.plainsScale, wz * (double)noise.plainsScale,
                                         noise.plainsOctaves, (double)noise.plainsLacunarity,
                                         (double)noise.plainsGain); // 0..1
            double mountain = perlin.ridged2D(wx * (double)noise.mountainScale, wz * (double)noise.mountainScale,
                                              noise.mountainOctaves, (double)noise.mountainLacunarity,
                                              (double)noise.mountainGain); // 0..1
            // 山脊锐度提升
            mountain = std::pow(std::clamp(mountain, 0.0, 1.0), 1.0 / std::max(0.1, (double)noise.ridgeSharpness));

            // 按群落混合
            // 细节层（打花纹 & 小起伏）
        double detail =
            perlin.fbm2D(wx * (double)noise.detailScale, wz * (double)noise.detailScale, 3, 2.0, 0.5);
            // 核心高度合成
            double h01 = plains * (1.0 - biome) + mountain * biome;       // 0..1
            h01 = h01 + (detail - 0.5) * 2.0 * (double)noise.detailAmp; // [-detailAmp, +detailAmp]
            h01 = std::clamp(h01, 0.0, 1.0);
            // 注入大陆大尺度抬升/压低
            h01 = (h01 * 0.8 + continentLift * 0.2);

            // 台地整形（可选）：将高度拉向 1/steps 的分段
            if (noise.plateau > 0.0f)
            {
                const double steps = std::max(1, noise.stepLevels);
                double stepped = std::floor(h01 * steps) / steps;
                h01 = h01 * (1.0 - (double)noise.plateau) + stepped * (double)noise.plateau;
            }

            // 高度分布调整：指数曲线（>1 提升高区占比）
            h01 = std::pow(std::clamp(h01, 0.0, 1.0), 1.0 / std::max(0.1, (double)noise.curveExponent));

            // 谷地下切：加强低处沟壑（适度）
            if (noise.valleyDepth > 0.0f)
            {
                double v = (0.5 - h01);
                h01 -= v * (double)noise.valleyDepth; // 低处更低，高处几乎不变
                h01 = std::clamp(h01, 0.0, 1.0);
            }

            // 海平面抬升
            h01 = std::max(h01, (double)noise.seaLevel);

            // 映射到体素高度
            double maxH = (double)(H - 1);
            int h = (int)glm::round(h01 * (double)noise.heightMul * maxH);
            h = glm::clamp(h, 0, H - 1);
            heightMap[z * W + x] = h;
        }
    }

    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    // 每顶点属性：pos(3) normal(3) color(4) uv(2) texIndex(1) tiling(1) => 14 floats
    vertices.reserve(W * D * H * 6 * 14 / 2);
    indices.reserve(W * D * H * 6);

    auto pushFace =
            [&](const glm::vec3 v[4], const std::array<glm::vec2, 4> &uv, uint32_t &baseIdx, const glm::vec3 &normal)
    {
        const float rC = 1.0f, gC = 1.0f, bC = 1.0f, aC = 1.0f, texIndex = 0.0f, tiling = 1.0f;
        for (int i = 0; i < 4; ++i)
        {
            vertices.push_back(v[i].x);
            vertices.push_back(v[i].y);
            vertices.push_back(v[i].z);
            // normal
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            vertices.push_back(rC);
            vertices.push_back(gC);
            vertices.push_back(bC);
            vertices.push_back(aC);
            vertices.push_back(uv[i].x);
            vertices.push_back(uv[i].y);
            vertices.push_back(texIndex);
            vertices.push_back(tiling);
        }
        indices.push_back(baseIdx + 0);
        indices.push_back(baseIdx + 1);
        indices.push_back(baseIdx + 2);
        indices.push_back(baseIdx + 2);
        indices.push_back(baseIdx + 3);
        indices.push_back(baseIdx + 0);
        baseIdx += 4;
    };

    // 在本作用域定义基于 Texture2D 图集 API 的 UV 计算（按归一 padding）
    auto makeQuadUV2 = [&](int col, int row) -> std::array<glm::vec2, 4>
    {
        float pad = m_GridPadding;
        return m_Atlas->GetUVFromGrid(col, row, m_AtlasCols, m_AtlasRows, pad);
    };

    auto uvFor = [&](BlockType t, int face) -> std::array<glm::vec2, 4>
    {
        AtlasTile tile{0, 0};
        switch (t)
        {
            case GRASS: tile = (face == 4 ? m_GrassUV.top : face == 5 ? m_GrassUV.bottom : m_GrassUV.side); break;
            case DIRT:  tile = m_DirtUV.side; break;
            case STONE: tile = m_StoneUV.side; break;
            default: break;
        }
    return makeQuadUV2(tile.col, tile.row);
    };

    uint32_t baseIdx = 0;
    const float s = 0.5f;
    for (int z = 0; z < D; ++z)
        for (int x = 0; x < W; ++x)
        {
            int h = heightMap[z * W + x];
            for (int y = 0; y <= h; ++y)
            {
                BlockType bt = (y == h) ? GRASS : (y >= h - 3 ? DIRT : STONE);

                bool posX = (x + 1 >= W) || (heightMap[z * W + (x + 1)] < y);
                bool negX = (x - 1 < 0) || (heightMap[z * W + (x - 1)] < y);
                bool posZ = (z + 1 >= D) || (heightMap[(z + 1) * W + x] < y);
                bool negZ = (z - 1 < 0) || (heightMap[(z - 1) * W + x] < y);
                bool top = (y + 1 > h);
                bool down = (y == 0);

                float cx = (float)x, cy = (float)y, cz = (float)z;
                glm::vec3 vF[4] = {{cx - s, cy - s, cz + s},
                                   {cx + s, cy - s, cz + s},
                                   {cx + s, cy + s, cz + s},
                                   {cx - s, cy + s, cz + s}};
                glm::vec3 vB[4] = {{cx + s, cy - s, cz - s},
                                   {cx - s, cy - s, cz - s},
                                   {cx - s, cy + s, cz - s},
                                   {cx + s, cy + s, cz - s}};
                glm::vec3 vL[4] = {{cx - s, cy - s, cz - s},
                                   {cx - s, cy - s, cz + s},
                                   {cx - s, cy + s, cz + s},
                                   {cx - s, cy + s, cz - s}};
                glm::vec3 vR[4] = {{cx + s, cy - s, cz + s},
                                   {cx + s, cy - s, cz - s},
                                   {cx + s, cy + s, cz - s},
                                   {cx + s, cy + s, cz + s}};
                glm::vec3 vT[4] = {{cx - s, cy + s, cz + s},
                                   {cx + s, cy + s, cz + s},
                                   {cx + s, cy + s, cz - s},
                                   {cx - s, cy + s, cz - s}};
                glm::vec3 vD_[4] = {{cx - s, cy - s, cz - s},
                                    {cx + s, cy - s, cz - s},
                                    {cx + s, cy - s, cz + s},
                                    {cx - s, cy - s, cz + s}};

                if (posZ)
                    pushFace(vF, uvFor(bt, 0), baseIdx, {0.0f, 0.0f, 1.0f});
                if (negZ)
                    pushFace(vB, uvFor(bt, 1), baseIdx, {0.0f, 0.0f, -1.0f});
                if (negX)
                    pushFace(vL, uvFor(bt, 2), baseIdx, {-1.0f, 0.0f, 0.0f});
                if (posX)
                    pushFace(vR, uvFor(bt, 3), baseIdx, {1.0f, 0.0f, 0.0f});
                if (top)
                    pushFace(vT, uvFor(bt, 4), baseIdx, {0.0f, 1.0f, 0.0f});
                if (down)
                    pushFace(vD_, uvFor(bt, 5), baseIdx, {0.0f, -1.0f, 0.0f});
            }
        }

    if (vertices.empty() || indices.empty())
        return;

    auto vb = Himii::VertexBuffer::Create(vertices.data(), (uint32_t)(vertices.size() * sizeof(float)));
    Himii::BufferLayout layout = {
            {Himii::ShaderDataType::Float3, "a_Position"}, {Himii::ShaderDataType::Float3, "a_Normal"},
            {Himii::ShaderDataType::Float4, "a_Color"},    {Himii::ShaderDataType::Float2, "a_TexCoord"},
            {Himii::ShaderDataType::Float, "a_TexIndex"},  {Himii::ShaderDataType::Float, "a_TilingFactor"},
    };
    vb->SetLayout(layout);
    m_TerrainVA->AddVertexBuffer(vb);

    auto ib = Himii::IndexBuffer::Create(indices.data(), static_cast<uint32_t>(indices.size()));
    m_TerrainVA->SetIndexBuffer(ib);
}

void CubeLayer::BuildSkybox()
{
    // 一个单位立方体，位置在原点，放大由着色器矩阵控制；仅位置属性
    m_SkyboxVA = Himii::VertexArray::Create();
    const float s = 1.0f;
    const glm::vec3 p[] = {
            {-s, -s, s},  {s, -s, s},   {s, s, s},   {-s, s, s},  // front
            {s, -s, -s},  {-s, -s, -s}, {-s, s, -s}, {s, s, -s},  // back
            {-s, -s, -s}, {-s, -s, s},  {-s, s, s},  {-s, s, -s}, // left
            {s, -s, s},   {s, -s, -s},  {s, s, -s},  {s, s, s},   // right
            {-s, s, s},   {s, s, s},    {s, s, -s},  {-s, s, -s}, // top
            {-s, -s, -s}, {s, -s, -s},  {s, -s, s},  {-s, -s, s}  // bottom
    };
    std::vector<float> verts;
    verts.reserve(24 * 3);
    for (auto &v: p)
    {
        verts.push_back(v.x);
        verts.push_back(v.y);
        verts.push_back(v.z);
    }
    std::vector<uint32_t> idx;
    for (uint32_t f = 0; f < 6; ++f)
    {
        uint32_t b = f * 4;
        idx.insert(idx.end(), {b + 0, b + 1, b + 2, b + 2, b + 3, b + 0});
    }
    auto vb = Himii::VertexBuffer::Create(verts.data(), (uint32_t)(verts.size() * sizeof(float)));
    vb->SetLayout({{Himii::ShaderDataType::Float3, "a_Position"}});
    m_SkyboxVA->AddVertexBuffer(vb);
    auto ib = Himii::IndexBuffer::Create(idx.data(), (uint32_t)idx.size());
    m_SkyboxVA->SetIndexBuffer(ib);
}

void CubeLayer::BuildECSScene()
{
    // 创建或刷新地形实体
    if (!m_TerrainEntity)
        m_TerrainEntity = m_Scene.CreateEntity("Terrain");
    if (!m_TerrainEntity.HasComponent<Himii::Transform>())
        m_TerrainEntity.AddComponent<Himii::Transform>();
    if (!m_TerrainEntity.HasComponent<Himii::MeshRenderer>())
        m_TerrainEntity.AddComponent<Himii::MeshRenderer>();
    // 绑定 TerrainScript
    if (!m_TerrainEntity.HasComponent<Himii::NativeScriptComponent>())
    {
        auto &nsc = m_TerrainEntity.AddComponent<Himii::NativeScriptComponent>();
        nsc.Bind<TerrainScript>();
    }
    else
    {
        auto &nsc = m_TerrainEntity.GetComponent<Himii::NativeScriptComponent>();
        if (!nsc.InstantiateScript) nsc.Bind<TerrainScript>();
    }
    {
        auto &mr = m_TerrainEntity.GetComponent<Himii::MeshRenderer>();
        mr.vertexArray = m_TerrainVA;
        mr.shader = m_LitShader;
        mr.texture = m_Atlas;
    }

    // 创建或刷新天空盒实体
    if (!m_SkyboxEntity)
        m_SkyboxEntity = m_Scene.CreateEntity("Skybox");
    if (!m_SkyboxEntity.HasComponent<Himii::Transform>())
        m_SkyboxEntity.AddComponent<Himii::Transform>();
    if (!m_SkyboxEntity.HasComponent<Himii::MeshRenderer>())
        m_SkyboxEntity.AddComponent<Himii::MeshRenderer>();
    if (!m_SkyboxEntity.HasComponent<Himii::SkyboxTag>())
        m_SkyboxEntity.AddComponent<Himii::SkyboxTag>();
    {
        auto &mr = m_SkyboxEntity.GetComponent<Himii::MeshRenderer>();
        mr.vertexArray = m_SkyboxVA;
        mr.shader = m_SkyboxShader;
        mr.texture = {};
    }

    // 如果场景中没有摄像机，则创建一个临时摄像机实体，承载原层相机参数（暂时）
    {
        auto viewCam = m_Scene.Registry().view<Himii::CameraComponent>();
        if (viewCam.begin() == viewCam.end())
        {
            auto camEnt = m_Scene.CreateEntity("Camera");
            auto &cc = camEnt.AddComponent<Himii::CameraComponent>();
            cc.primary = true;
            cc.projection = Himii::ProjectionType::Perspective;
            cc.fovYDeg = m_FovYDeg;
            cc.nearZ = m_NearZ;
            cc.farZ = m_FarZ;
            cc.useLookAt = true;
            cc.lookAtTarget = m_CamTarget;
            auto &tr = camEnt.GetComponent<Himii::Transform>();
            tr.Position = m_CamPos;
        }
    }
}
