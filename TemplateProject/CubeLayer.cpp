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

    // 0) 创建离屏帧缓冲（Scene/Game），初始尺寸给一个合理默认，后续由 Editor 面板驱动 Resize
    {
        Himii::FramebufferSpecification fbSpec;
        fbSpec.Width = 1280; fbSpec.Height = 720;
        m_Framebuffer = Himii::Framebuffer::Create(fbSpec);
        m_GameFramebuffer = Himii::Framebuffer::Create(fbSpec);
    }

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
            aspectScene = (float)newW / (float)newH;
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

    // 相机输入更新：仅在 Scene 面板聚焦/悬停时接收输入
    if (!editorRef || editorRef->IsSceneHovered() || editorRef->IsSceneFocused())
        UpdateCamera(ts);

    // 渲染函数：把一帧画到指定帧缓冲（ECS 提交在 Scene::OnUpdate 内部）
    auto renderTo = [this](Himii::Ref<Himii::Framebuffer> fb, float aspect)
    {
        if (!fb)
            return;
        fb->Bind();
        Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
        Himii::RenderCommand::Clear();

        glm::mat4 transform(1.0f);
        // 计算透视投影与视图矩阵（使用传入 aspect）
        const float fovYRad = glm::radians(m_FovYDeg);
    glm::mat4 projection = glm::perspective<float>(fovYRad, aspect, m_NearZ, m_FarZ); 
        glm::mat4 view = glm::lookAt(m_CamPos, m_CamTarget, m_CamUp);
        glm::mat4 viewProj = projection * view;

        // 将每帧可变数据（比如天空盒围绕相机的缩放/旋转）写入到对应实体的 Transform 上
        if (m_SkyboxEntity)
        {
            auto &tr = m_SkyboxEntity.GetComponent<Himii::Transform>();
            tr.Position = m_CamPos; // 天空盒跟随相机位置
            tr.Scale = glm::vec3(m_FarZ * 0.5f);
            tr.Rotation = {0.0f, glm::radians(m_Angle), 0.0f};
        }

        // 一些全局材质/灯光参数，通过 Shader 的外部接口设置一次
        if (m_LitShader)
        {
            m_LitShader->Bind();
            m_LitShader->SetFloat3("u_AmbientColor", m_AmbientColor);
            m_LitShader->SetFloat("u_AmbientIntensity", m_AmbientIntensity);
            m_LitShader->SetFloat3("u_LightDir", m_LightDir);
            m_LitShader->SetFloat3("u_LightColor", m_LightColor);
            m_LitShader->SetFloat("u_LightIntensity", m_LightIntensity);
        }

        // 由 Renderer 管线包裹 ECS 的渲染
        Himii::Renderer::BeginScene(viewProj);
        m_Scene.OnUpdate({});
        Himii::Renderer::EndScene();
        fb->Unbind();
    };

    // 渲染到两个目标
    renderTo(m_Framebuffer, aspectScene > 0.0f ? aspectScene : m_Aspect);
    renderTo(m_GameFramebuffer, aspectGame > 0.0f ? aspectGame : m_Aspect);
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

    if (Input::IsKeyPressed(Key::W))
        m_CamPos += front * speed * dt;
    if (Input::IsKeyPressed(Key::S))
        m_CamPos -= front * speed * dt;
    if (Input::IsKeyPressed(Key::A))
        m_CamPos -= right * speed * dt;
    if (Input::IsKeyPressed(Key::D))
        m_CamPos += right * speed * dt;
    if (Input::IsKeyPressed(Key::Q))
        m_CamPos -= up * speed * dt;
    if (Input::IsKeyPressed(Key::E))
        m_CamPos += up * speed * dt;

    // 更新 Target 与 Up
    m_CamTarget = m_CamPos + front;
    m_CamUp = up;
}

// OnImGuiRender 已移除

void CubeLayer::BuildTerrainMesh()
{
    // 为避免缓冲区尺寸不足导致 glBufferSubData 失败，这里直接重建 VA/VB/IB
    m_TerrainVA = Himii::VertexArray::Create();

    PerlinNoise perlin(m_Noise.seed);
    const int W = m_TerrainW, D = m_TerrainD, H = m_TerrainH;
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
            perlin.domainWarp2D(wx, wz, (double)m_Noise.warpScale, (double)m_Noise.warpAmp);

            // 低频群落权重（0..1），调制山地比重
            double biome = perlin.fbm2D(nx * (double)m_Noise.biomeScale, nz * (double)m_Noise.biomeScale, 3, 2.0, 0.5);
            biome = std::clamp(biome * (double)m_Noise.mountainWeight, 0.0, 1.0);

            // 大洲层：决定“大陆-海洋”的宏观起伏
            double continent = perlin.fbm2D(nx * (double)m_Noise.continentScale, nz * (double)m_Noise.continentScale, 4,
                                            2.0, 0.5); // 0..1
            continent = (continent * 2.0 - 1.0);       // [-1,1]
            continent = std::clamp(continent * (double)m_Noise.continentStrength, -1.0, 1.0);
            // 映射至 0..1，正值抬升，负值压低
            double continentLift = (continent + 1.0) * 0.5; // 0..1

            // 平原（柔和）与山地（脊状）两套噪声
            double plains = perlin.fbm2D(wx * (double)m_Noise.plainsScale, wz * (double)m_Noise.plainsScale,
                                         m_Noise.plainsOctaves, (double)m_Noise.plainsLacunarity,
                                         (double)m_Noise.plainsGain); // 0..1
            double mountain = perlin.ridged2D(wx * (double)m_Noise.mountainScale, wz * (double)m_Noise.mountainScale,
                                              m_Noise.mountainOctaves, (double)m_Noise.mountainLacunarity,
                                              (double)m_Noise.mountainGain); // 0..1
            // 山脊锐度提升
            mountain = std::pow(std::clamp(mountain, 0.0, 1.0), 1.0 / std::max(0.1, (double)m_Noise.ridgeSharpness));

            // 按群落混合
            // 细节层（打花纹 & 小起伏）
            double detail =
                    perlin.fbm2D(wx * (double)m_Noise.detailScale, wz * (double)m_Noise.detailScale, 3, 2.0, 0.5);
            // 核心高度合成
            double h01 = plains * (1.0 - biome) + mountain * biome;       // 0..1
            h01 = h01 + (detail - 0.5) * 2.0 * (double)m_Noise.detailAmp; // [-detailAmp, +detailAmp]
            h01 = std::clamp(h01, 0.0, 1.0);
            // 注入大陆大尺度抬升/压低
            h01 = (h01 * 0.8 + continentLift * 0.2);

            // 台地整形（可选）：将高度拉向 1/steps 的分段
            if (m_Noise.plateau > 0.0f)
            {
                const double steps = std::max(1, m_Noise.stepLevels);
                double stepped = std::floor(h01 * steps) / steps;
                h01 = h01 * (1.0 - (double)m_Noise.plateau) + stepped * (double)m_Noise.plateau;
            }

            // 高度分布调整：指数曲线（>1 提升高区占比）
            h01 = std::pow(std::clamp(h01, 0.0, 1.0), 1.0 / std::max(0.1, (double)m_Noise.curveExponent));

            // 谷地下切：加强低处沟壑（适度）
            if (m_Noise.valleyDepth > 0.0f)
            {
                double v = (0.5 - h01);
                h01 -= v * (double)m_Noise.valleyDepth; // 低处更低，高处几乎不变
                h01 = std::clamp(h01, 0.0, 1.0);
            }

            // 海平面抬升
            h01 = std::max(h01, (double)m_Noise.seaLevel);

            // 映射到体素高度
            double maxH = (double)(H - 1);
            int h = (int)glm::round(h01 * (double)m_Noise.heightMul * maxH);
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
}
